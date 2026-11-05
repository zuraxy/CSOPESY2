#include "console.hpp"
#include "scheduler.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <cerrno>
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

namespace csopesy {
namespace {

using namespace std::chrono_literals;
constexpr const char* prompt = "root:\\> ";

class Runtime {
public:
    Runtime(Config config, ClockPolicy policy)
        : scheduler_(config), policy_(policy), period_(policy.generating),
          clock_([this] { run(); }) {}

    ~Runtime() {
        {
            const std::lock_guard lock(mutex_);
            stopping_ = true;
        }
        wake_.notify_one();
        clock_.join();
    }

    template <typename Action>
    auto access(Action action) {
        const std::lock_guard lock(mutex_);
        if (failure_) {
            std::rethrow_exception(failure_);
        }
        return action(scheduler_);
    }

    void generation(bool enabled) {
        {
            const std::lock_guard lock(mutex_);
            if (failure_) {
                std::rethrow_exception(failure_);
            }
            if (enabled) {
                scheduler_.startBatch();
            } else {
                scheduler_.stopBatch();
            }
            const auto period = enabled ? policy_.generating : policy_.stopped;
            if (period != period_) {
                period_ = period;
                changed_ = true;
            }
        }
        wake_.notify_one();
    }

private:
    void run() {
        std::unique_lock lock(mutex_);
        auto deadline = std::chrono::steady_clock::now() + period_;
        try {
            while (!stopping_) {
                if (wake_.wait_until(lock, deadline, [this] { return stopping_ || changed_; })) {
                    if (changed_) {
                        changed_ = false;
                        deadline = std::chrono::steady_clock::now() + period_;
                    }
                    continue;
                }
                scheduler_.step();
                deadline += period_;
                // Avoid an unbounded catch-up burst if the host was suspended.
                // No logical ticks or process instructions are skipped.
                const auto now = std::chrono::steady_clock::now();
                if (now - deadline > 100ms) {
                    deadline = now + period_;
                }
            }
        } catch (...) {
            failure_ = std::current_exception();
        }
    }

    Scheduler scheduler_;
    ClockPolicy policy_;
    std::chrono::milliseconds period_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_ = false;
    bool changed_ = false;
    std::exception_ptr failure_;
    std::thread clock_;
};

// Input and output stay on the UI thread. Polling allows attached screens to
// show new PRINT logs even when the user has not entered another command.
class ConsoleInput {
public:
    ConsoleInput() {
#ifdef _WIN32
        input_ = GetStdHandle(STD_INPUT_HANDLE);
        if (GetConsoleMode(input_, &oldMode_)) {
            if (!SetConsoleMode(input_, oldMode_ & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT))) {
                throw std::runtime_error("Could not configure console input.");
            }
            interactive_ = true;
        }
        kind_ = GetFileType(input_);
#else
        if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &oldMode_) == 0) {
            auto mode = oldMode_;
            mode.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            mode.c_cc[VMIN] = 0;
            mode.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) != 0) {
                throw std::runtime_error("Could not configure console input.");
            }
            interactive_ = true;
        }
#endif
    }

    ~ConsoleInput() {
        if (interactive_) {
#ifdef _WIN32
            SetConsoleMode(input_, oldMode_);
#else
            tcsetattr(STDIN_FILENO, TCSANOW, &oldMode_);
#endif
        }
    }

    bool interactive() const { return interactive_; }
    bool closed() const { return ended_ && lines_.empty() && text_.empty(); }
    const std::string& text() const { return text_; }

    std::optional<std::string> poll() {
        if (!lines_.empty()) {
            return takeLine();
        }
        if (!pending_.empty()) {
            acceptPending();
            if (!lines_.empty()) {
                return takeLine();
            }
        }
        if (ended_) {
            if (!text_.empty()) {
                auto last = std::move(text_);
                text_.clear();
                return last;
            }
            return {};
        }
#ifdef _WIN32
        if (interactive_) {
            const auto status = WaitForSingleObject(input_, 20);
            if (status == WAIT_OBJECT_0) {
                INPUT_RECORD records[32];
                DWORD count = 0;
                if (!ReadConsoleInputA(input_, records, 32, &count)) {
                    throw std::runtime_error("Could not read console input.");
                }
                for (DWORD i = 0; i < count; ++i) {
                    const auto& record = records[i];
                    if (record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown) {
                        const auto& key = record.Event.KeyEvent;
                        if (key.uChar.AsciiChar != 0) {
                            for (WORD j = 0; j < key.wRepeatCount; ++j) {
                                pending_.push_back(key.uChar.AsciiChar);
                            }
                        }
                    }
                }
                acceptPending();
            } else if (status == WAIT_FAILED) {
                throw std::runtime_error("Could not wait for console input.");
            }
        } else {
            DWORD available = 4096;
            if (kind_ == FILE_TYPE_PIPE && !PeekNamedPipe(input_, nullptr, 0, nullptr, &available, nullptr)) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    ended_ = true;
                } else {
                    throw std::runtime_error("Could not read redirected input.");
                }
            }
            if (!ended_ && available != 0) {
                char buffer[4096];
                DWORD count = 0;
                if (!ReadFile(input_, buffer, std::min<DWORD>(available, 4096), &count, nullptr)) {
                    if (GetLastError() == ERROR_BROKEN_PIPE) {
                        ended_ = true;
                    } else {
                        throw std::runtime_error("Could not read redirected input.");
                    }
                } else if (count == 0) {
                    ended_ = true;
                } else {
                    for (DWORD i = 0; i < count; ++i) {
                        accept(buffer[i]);
                    }
                }
            } else if (!ended_) {
                std::this_thread::sleep_for(20ms);
            }
        }
#else
        pollfd descriptor{STDIN_FILENO, POLLIN, 0};
        const auto status = ::poll(&descriptor, 1, 20);
        if (status < 0 && errno != EINTR) {
            throw std::runtime_error("Could not wait for input.");
        }
        if (status > 0) {
            char buffer[4096];
            const auto count = ::read(STDIN_FILENO, buffer, sizeof buffer);
            if (count == 0) {
                ended_ = true;
            } else if (count > 0) {
                for (ssize_t i = 0; i < count; ++i) {
                    accept(buffer[i]);
                }
            } else if (errno != EINTR) {
                throw std::runtime_error("Could not read input.");
            }
        }
#endif
        if (!lines_.empty()) {
            return takeLine();
        }
        if (ended_ && !text_.empty()) {
            auto last = std::move(text_);
            text_.clear();
            return last;
        }
        return {};
    }

private:
    void acceptPending() {
        // Process pasted console commands one line at a time, after the previous
        // command's response/prompt. This avoids echoing the next line mid-response.
        while (!pending_.empty() && lines_.empty()) {
            const char ch = pending_.front();
            pending_.pop_front();
            accept(ch);
        }
    }

    std::optional<std::string> takeLine() {
        auto line = std::move(lines_.front());
        lines_.pop_front();
        return line;
    }

    void accept(char ch) {
        if (ch == '\r') {
            // Native console Enter arrives as CR; redirected Windows lines are CRLF.
            if (!interactive_) {
                return;
            }
            ch = '\n';
        }
        if (interactive_ && (ch == '\x04' || ch == '\x1a')) {
            ended_ = true;
        } else if (ch == '\n') {
            lines_.push_back(std::move(text_));
            text_.clear();
            if (interactive_) {
                std::cout << '\n';
            }
        } else if (interactive_ && (ch == '\b' || ch == '\x7f')) {
            if (!text_.empty()) {
                text_.pop_back();
                std::cout << "\b \b";
            }
        } else if (!interactive_ || static_cast<unsigned char>(ch) >= 32 || ch == '\t') {
            text_ += ch;
            if (interactive_) {
                std::cout << ch;
            }
        }
        if (interactive_) {
            std::cout.flush();
        }
    }

    bool interactive_ = false;
    bool ended_ = false;
    std::string text_;
    std::deque<std::string> lines_;
    std::deque<char> pending_;
#ifdef _WIN32
    HANDLE input_ = INVALID_HANDLE_VALUE;
    DWORD oldMode_ = 0;
    DWORD kind_ = FILE_TYPE_UNKNOWN;
#else
    termios oldMode_{};
#endif
};

bool enableConsoleClear() {
#ifdef _WIN32
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    return GetConsoleMode(output, &mode)
        && SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

void clearConsole(bool enabled) {
    if (enabled) {
        std::cout << "\x1b[2J\x1b[H";
    }
}

std::filesystem::path executableDirectory(const char* argument) {
#ifdef _WIN32
    std::vector<wchar_t> buffer(512);
    for (;;) {
        const DWORD count = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (count == 0) {
            throw std::runtime_error("Could not locate the executable.");
        }
        if (count < buffer.size()) {
            return std::filesystem::path(std::wstring(buffer.data(), count)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__linux__)
    std::vector<char> buffer(512);
    for (;;) {
        const auto count = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (count < 0) {
            break;
        }
        if (static_cast<std::size_t>(count) < buffer.size()) {
            return std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(count))).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#elif defined(__APPLE__)
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::vector<char> buffer(size);
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        return std::filesystem::weakly_canonical(buffer.data()).parent_path();
    }
#endif
    return std::filesystem::absolute(argument).parent_path();
}

struct Options {
    std::filesystem::path config;
    Workload workload = Workload::random;
    bool help = false;
};

Options options(int argc, char** argv) {
    Options result{executableDirectory(argv[0]) / "config.txt", Workload::random, false};
    for (int i = 1; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--help") {
            result.help = true;
        } else if ((option == "--config" || option == "--workload") && i + 1 < argc) {
            const std::string value = argv[++i];
            if (option == "--config") {
                result.config = std::filesystem::absolute(value).lexically_normal();
            } else if (value == "random") {
                result.workload = Workload::random;
            } else if (value == "alternating") {
                result.workload = Workload::alternating;
            } else {
                throw std::invalid_argument("--workload must be random or alternating.");
            }
        } else {
            throw std::invalid_argument("Usage: csopesy [--config <file>] [--workload random|alternating]");
        }
    }
    return result;
}

void printBanner() {
    std::cout << R"(  ____ ____   ___  ____  _____ ______   __
 / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /
| |   \___ \| | | | |_) |  _| \___ \\ V /
| |___ ___) | |_| |  __/| |___ ___) || |
 \____|____/ \___/|_|   |_____|____/ |_|
-------------------------------------------
Welcome to CSOPESY Emulator!

Developers:
Aquino, Bon
Dela Cruz, Karl Matthew
Espinosa, Jose Miguel
Pineda, Dencel Angelo

Last updated: 11-05-2026
-------------------------------------------

)";
}

std::string timestamp(Time time) {
    const auto localTime = std::chrono::system_clock::to_time_t(time);
    std::ostringstream text;
    text << std::put_time(std::localtime(&localTime), "%m/%d/%Y %I:%M:%S%p");
    return text.str();
}

void printLogs(const ProcessScreen& screen) {
    for (const auto& entry : screen.logs) {
        std::cout << '(' << timestamp(entry.time) << ") Core:" << entry.core
                  << ' ' << std::quoted(entry.message) << '\n';
    }
}

void printProgress(const ProcessSummary& process) {
    if (process.state == State::finished) {
        std::cout << "\nFinished!\n\n";
    } else {
        std::cout << "\nCurrent instruction line: " << process.completed
                  << "\nLines of code: " << process.total << "\n\n";
    }
}

void showScreen(const ProcessScreen& screen) {
    std::cout << "Process name: " << screen.process.name << "\nID: "
              << screen.process.id + 1 << "\nLogs:\n";
    printLogs(screen);
    printProgress(screen.process);
}

void writeUtilization(std::ostream& out, const Utilization& report) {
    std::ostringstream percentage;
    const auto scaled = report.used * 100;
    if (scaled % report.cores == 0) {
        percentage << scaled / report.cores;
    } else {
        percentage << std::fixed << std::setprecision(2)
                   << static_cast<double>(scaled) / static_cast<double>(report.cores);
    }
    out << "CPU utilization: " << percentage.str() << "%\n"
        << "Cores used: " << report.used << "\n"
        << "Cores available: " << report.cores - report.used << "\n\n"
        << "-------------------------------------------\nRunning processes:\n";
    for (const auto& process : report.running) {
        out << process.name << " (" << timestamp(process.created) << ")\tCore: " << *process.core
            << "\t" << process.completed << " / " << process.total << '\n';
    }
    out << "\nFinished processes:\n";
    for (const auto& process : report.finished) {
        out << process.name << " (" << timestamp(process.created) << ")\tFinished"
            << "\t" << process.completed << " / " << process.total << '\n';
    }
    const auto waiting = [&out](const char* heading, const auto& processes, const char* state) {
        out << '\n' << heading << ":\n";
        for (const auto& process : processes) {
            out << process.name << " (" << timestamp(process.created) << ")\t" << state
                << "\t" << process.completed << " / " << process.total << '\n';
        }
    };
    waiting("Ready processes", report.ready, "Ready");
    waiting("Sleeping processes", report.sleeping, "Sleeping");
    out << "-------------------------------------------\n";
}

void saveReport(const Utilization& report, const std::filesystem::path& config) {
    const auto path = config.parent_path() / "csopesy-log.txt";
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open " + path.string() + " for writing.");
    }
    writeUtilization(file, report);
    file.close();
    if (!file) {
        throw std::runtime_error("Could not save " + path.string() + ".");
    }
    std::cout << "Report generated at " << path.string() << "!\n";
}

std::vector<std::string> tokens(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> result;
    std::string word;
    while (input >> word) {
        result.push_back(word);
    }
    return result;
}

} // namespace

int runConsole(int argc, char** argv, ClockPolicy policy) {
    try {
        const auto settings = options(argc, argv);
        if (settings.help) {
            std::cout << "Usage: csopesy [--config <file>] [--workload random|alternating]\n"
                      << "Default config: config.txt beside the executable.\n"
                      << "Question 5: use --workload alternating.\n";
            return 0;
        }
        ConsoleInput input;
        const bool clear = enableConsoleClear();
        std::unique_ptr<Runtime> runtime;
        std::optional<ProcessId> current;
        std::size_t shownLogs = 0;
        bool finishedShown = false;
        auto nextRefresh = std::chrono::steady_clock::now();

        printBanner();
        std::cout << "Configuration: " << settings.config.string() << '\n'
                  << "Workload: " << (settings.workload == Workload::alternating ? "alternating" : "random") << '\n';
        if (policy.explanation) {
            std::cout << policy.explanation << '\n';
        }
        std::cout << '\n' << prompt << std::flush;
        for (;;) {
            if (const auto line = input.poll()) {
                auto command = tokens(*line);
                if (command.size() == 1 && command[0] == "screen-ls") {
                    command = {"screen", "-ls"};
                }
                if (!command.empty()) {
                    const auto& name = command[0];
                    const bool noArgs = command.size() == 1;
                    try {
                        if (current) {
                            if (noArgs && name == "exit") {
                                current.reset();
                                clearConsole(clear);
                                printBanner();
                            } else if (noArgs && name == "process-smi") {
                                const auto screen = runtime->access([&](Scheduler& scheduler) {
                                    return scheduler.screen(*current);
                                });
                                std::cout << '\n';
                                showScreen(screen);
                                shownLogs = screen.logs.size();
                                finishedShown = screen.process.state == State::finished;
                            } else {
                                std::cout << "Available commands: process-smi, exit\n";
                            }
                        } else if (noArgs && name == "exit") {
                            break;
                        } else if (noArgs && name == "initialize") {
                            if (runtime) {
                                std::cout << "The emulator is already initialized.\n";
                            } else {
                                auto config = readConfig(settings.config, std::cout);
                                config.workload = settings.workload;
                                runtime = std::make_unique<Runtime>(config, policy);
                                std::cout << "Initialized.\n";
                            }
                        } else if (!runtime) {
                            std::cout << "Run initialize first.\n";
                        } else if (name == "screen") {
                            if (command.size() == 2 && command[1] == "-ls") {
                                writeUtilization(std::cout, runtime->access([](Scheduler& scheduler) {
                                    return scheduler.utilization();
                                }));
                                std::cout << '\n';
                            } else if (command.size() == 3 && (command[1] == "-s" || command[1] == "-r")) {
                                current = runtime->access([&](Scheduler& scheduler) -> std::optional<ProcessId> {
                                    return command[1] == "-s"
                                        ? std::optional<ProcessId>{scheduler.create(command[2])}
                                        : scheduler.findLive(command[2]);
                                });
                                if (current) {
                                    const auto screen = runtime->access([&](Scheduler& scheduler) {
                                        return scheduler.screen(*current);
                                    });
                                    clearConsole(clear);
                                    showScreen(screen);
                                    shownLogs = screen.logs.size();
                                    finishedShown = screen.process.state == State::finished;
                                } else {
                                    std::cout << "Process " << command[2] << " not found.\n";
                                }
                            } else {
                                std::cout << "Usage: screen -s <process name>, screen -r <process name>, or screen -ls\n";
                            }
                        } else if (noArgs && (name == "scheduler-start" || name == "scheduler-test")) {
                            runtime->generation(true);
                            std::cout << "Batch generation started.\n";
                        } else if (noArgs && name == "scheduler-stop") {
                            runtime->generation(false);
                            std::cout << "Batch generation stopped.\n";
                        } else if (noArgs && name == "report-util") {
                            saveReport(runtime->access([](Scheduler& scheduler) {
                                return scheduler.utilization();
                            }), settings.config);
                        } else {
                            std::cout << "Unknown command.\n";
                        }
                    } catch (const std::exception& error) {
                        std::cout << error.what() << '\n';
                    }
                }
                std::cout << prompt << std::flush;
            } else if (input.closed()) {
                break;
            }

            if (current && !finishedShown && std::chrono::steady_clock::now() >= nextRefresh) {
                // Batch visual updates, without changing scheduler tick cadence.
                nextRefresh = std::chrono::steady_clock::now() + 100ms;
                const auto screen = runtime->access([&](Scheduler& scheduler) {
                    return scheduler.screen(*current, shownLogs);
                });
                if (!screen.logs.empty() || screen.process.state == State::finished) {
                    if (input.interactive()) {
                        if (clear) {
                            std::cout << "\r\x1b[2K";
                        } else {
                            std::cout << '\r' << std::string(std::string(prompt).size() + input.text().size(), ' ') << '\r';
                        }
                    } else {
                        std::cout << '\n';
                    }
                    printLogs(screen);
                    printProgress(screen.process);
                    shownLogs += screen.logs.size();
                    finishedShown = screen.process.state == State::finished;
                    std::cout << prompt << input.text() << std::flush;
                }
            }
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

} // namespace csopesy
