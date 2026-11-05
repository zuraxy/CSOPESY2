#include "scheduler.hpp"

#include <chrono>
#include <condition_variable>
#include <ctime>
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
#include <unistd.h>
#endif

namespace {

using namespace csopesy;
using namespace std::chrono_literals;

constexpr auto tickInterval = 5ms;

class Runtime {
public:
    explicit Runtime(Config config) : scheduler_(config), clock_([this] { run(); }) {}

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
        return action(scheduler_);
    }

private:
    void run() {
        std::unique_lock lock(mutex_);
        while (!wake_.wait_for(lock, tickInterval, [this] { return stopping_; })) {
            scheduler_.step();
        }
    }

    Scheduler scheduler_;
    std::mutex mutex_;
    std::condition_variable wake_;
    bool stopping_ = false;
    std::thread clock_;
};

bool enableConsoleClear() {
#ifdef _WIN32
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
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

void printBanner() {
    std::cout << R"(  ____ ____   ___  ____  _____ ______   __
 / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /
| |   \___ \| | | | |_) |  _| \___ \\ V /
| |___ ___) | |_| |  __/| |___ ___) || |
 \____|____/ \___/|_|   |_____|____/ |_|
-------------------------------------------
Welcome to CSOPESY Emulator!

Developers:
Karl Matthew Dela Cruz, Bon Aquino,
Dencel Angelo Pineda, Jose Miguel Espinosa
-------------------------------------------
)";
}

std::string timestamp(Time time) {
    const std::time_t localTime = std::chrono::system_clock::to_time_t(time);
    std::ostringstream text;
    text << std::put_time(std::localtime(&localTime), "%m/%d/%Y %I:%M:%S%p");
    return text.str();
}

void showScreen(Runtime& runtime, ProcessId id) {
    const auto screen = runtime.access([id](Scheduler& scheduler) {
        return scheduler.screen(id);
    });
    const auto& process = screen.process;
    std::cout << "Process name: " << process.name << "\nID: " << process.id + 1 << "\nLogs:\n";
    for (const auto& entry : screen.logs) {
        std::cout << '(' << timestamp(entry.time) << ") Core:" << entry.core
                  << ' ' << std::quoted(entry.message) << '\n';
    }
    if (process.state == State::finished) {
        std::cout << "\nFinished!\n";
    } else {
        std::cout << "\nCurrent instruction line: " << process.completed
                  << "\nLines of code: " << process.total << '\n';
    }
}

void writeUtilization(std::ostream& out, const Utilization& report) {
    std::ostringstream percentage;
    const auto scaled = report.used * 100;
    if (scaled % report.cores == 0) {
        percentage << scaled / report.cores;
    } else {
        percentage << std::fixed << std::setprecision(2)
                   << static_cast<double>(scaled) / report.cores;
    }
    out << "CPU utilization: " << percentage.str() << "%\n"
        << "Cores used: " << report.used << "\n"
        << "Cores available: " << report.cores - report.used << "\n\n"
        << "-------------------------------------------\n"
        << "Running processes:\n";
    for (const auto& process : report.running) {
        out << process.name << " (" << timestamp(process.created) << ")\tCore: " << *process.core
            << "\t" << process.completed << " / " << process.total << '\n';
    }
    out << "\nFinished processes:\n";
    for (const auto& process : report.finished) {
        out << process.name << " (" << timestamp(process.created) << ")\tFinished"
            << "\t" << process.completed << " / " << process.total << '\n';
    }
    out << "-------------------------------------------\n";
}

void saveReport(const Utilization& report) {
    const auto path = std::filesystem::absolute("csopesy-log.txt");
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("Could not open csopesy-log.txt for writing.");
    }
    writeUtilization(file, report);
    file.close();
    if (!file) {
        throw std::runtime_error("Could not save csopesy-log.txt.");
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

void screenCommand(const std::vector<std::string>& command, Runtime& runtime,
                   std::optional<ProcessId>& current, bool clear) {
    if (command.size() == 2 && command[1] == "-ls") {
        const auto report = runtime.access([](Scheduler& scheduler) {
            return scheduler.utilization();
        });
        writeUtilization(std::cout, report);
        return;
    }
    if (command.size() != 3 || (command[1] != "-s" && command[1] != "-r")) {
        std::cout << "Usage: screen -s <process name>, screen -r <process name>, or screen -ls\n";
        return;
    }
    current = runtime.access([&command](Scheduler& scheduler) -> std::optional<ProcessId> {
        if (command[1] == "-s") {
            return scheduler.create(command[2]);
        }
        return scheduler.findLive(command[2]);
    });
    if (!current) {
        std::cout << "Process " << command[2] << " not found.\n";
        return;
    }
    clearConsole(clear);
    showScreen(runtime, *current);
}

} // namespace

int main() {
    std::unique_ptr<Runtime> runtime;
    std::optional<ProcessId> current;
    const bool clear = enableConsoleClear();
    printBanner();

    std::string line;
    while (std::cout << "root:\\> " && std::getline(std::cin, line)) {
        const auto command = tokens(line);
        if (command.empty()) {
            continue;
        }
        const auto& name = command[0];
        const bool noArgs = command.size() == 1;
        try {
            if (current) {
                if (noArgs && name == "exit") {
                    current.reset();
                    clearConsole(clear);
                    printBanner();
                } else if (noArgs && name == "process-smi") {
                    showScreen(*runtime, *current);
                } else {
                    std::cout << "Available commands: process-smi, exit\n";
                }
            } else if (noArgs && name == "exit") {
                break;
            } else if (noArgs && name == "initialize") {
                if (runtime) {
                    std::cout << "The emulator is already initialized.\n";
                } else {
                    runtime = std::make_unique<Runtime>(readConfig("config.txt", std::cout));
                    std::cout << "Initialized.\n";
                }
            } else if (!runtime) {
                std::cout << "Run initialize first.\n";
            } else if (name == "screen") {
                screenCommand(command, *runtime, current, clear);
            } else if (noArgs && name == "scheduler-start") {
                runtime->access([](Scheduler& scheduler) { scheduler.startBatch(); });
                std::cout << "Batch generation started.\n";
            } else if (noArgs && name == "scheduler-test") {
                std::cout << "no scheduler-test command found, did you mean scheduler-start?\n";
            } else if (noArgs && name == "scheduler-stop") {
                runtime->access([](Scheduler& scheduler) { scheduler.stopBatch(); });
                std::cout << "Batch generation stopped.\n";
            } else if (noArgs && name == "report-util") {
                saveReport(runtime->access([](Scheduler& scheduler) {
                    return scheduler.utilization();
                }));
            } else {
                std::cout << "Unknown command.\n";
            }
        } catch (const std::exception& error) {
            std::cout << error.what() << '\n';
        }
    }
}
