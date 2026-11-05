#include "scheduler.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace csopesy {
namespace {

constexpr Tick maxSetting = Tick{1} << 32;

Tick number(const std::string& value, const std::string& key, Tick low, Tick high,
            std::ostream& warnings) {
    Tick result = 0;
    const bool negative = !value.empty() && value.front() == '-';
    const bool signedValue = negative || (!value.empty() && value.front() == '+');
    const auto end = value.data() + value.size();
    const auto parsed = std::from_chars(value.data() + signedValue, end, result);
    const bool overflow = parsed.ec == std::errc::result_out_of_range;
    if (parsed.ptr != end || (parsed.ec != std::errc{} && !overflow)) {
        throw std::runtime_error("Invalid integer for " + key + ".");
    }
    if ((negative && (overflow || result != 0)) || result < low || result > high || overflow) {
        const Tick bound = negative || (!overflow && result < low) ? low : high;
        warnings << "Warning: " << key << " is out of range. Using " << bound << ".\n";
        return bound;
    }
    return result;
}

Instruction generateInstruction(std::minstd_rand& random, const std::string& name,
                                Tick& budget, unsigned depth = 0) {
    const auto pick = [&random](int low, int high) {
        return std::uniform_int_distribution<int>(low, high)(random);
    };
    const auto variable = [&pick] {
        return std::string(1, static_cast<char>('a' + pick(0, 25)));
    };
    // Each operand is independently a variable or a literal, covering
    // value+value, var+value, value+var and var+var.
    const auto operand = [&pick, &variable]() -> Operand {
        if (pick(0, 1) == 0) {
            return variable();
        }
        return static_cast<std::uint16_t>(pick(0, 100));
    };
    const int operation = pick(0, depth < 3 ? 5 : 4);
    if (operation == 5) {
        const auto repeats = static_cast<std::uint32_t>(
            pick(1, static_cast<int>(std::min<Tick>(3, budget))));
        const auto bodyTicks = static_cast<Tick>(
            pick(1, static_cast<int>(std::min<Tick>(3, budget / repeats))));
        For loop{{}, repeats};
        Tick remaining = bodyTicks;
        while (remaining != 0) {
            loop.body.push_back(generateInstruction(random, name, remaining, depth + 1));
        }
        budget -= bodyTicks * repeats;
        return {std::move(loop)};
    }

    --budget;
    switch (operation) {
    case 0:
        return {Sleep{static_cast<std::uint8_t>(pick(1, 5))}};
    case 1:
        return {Declare{variable(), static_cast<std::uint16_t>(pick(0, 100))}};
    case 2:
        return {Add{variable(), operand(), operand()}};
    case 3:
        return {Subtract{variable(), operand(), operand()}};
    default:
        return {Print{"Hello world from " + name + "!", std::nullopt}};
    }
}

Tick countInstructions(const Program& program, unsigned depth = 0) {
    Tick total = 0;
    for (const auto& instruction : program) {
        if (const auto* loop = std::get_if<For>(&instruction.operation)) {
            if (depth == 3) {
                throw std::invalid_argument("FOR supports up to three nested levels.");
            }
            const Tick body = countInstructions(loop->body, depth + 1);
            if (body != 0 && loop->repeats > (maxSetting - total) / body) {
                throw std::invalid_argument("Process has too many instructions.");
            }
            total += body * loop->repeats;
        } else {
            if (total == maxSetting) {
                throw std::invalid_argument("Process has too many instructions.");
            }
            ++total;
        }
    }
    return total;
}

struct LoopFrame {
    const For* loop;
    std::size_t next = 0;
    std::uint32_t iteration = 0;
};

struct Execution {
    Execution(std::optional<Program> instructions, std::uint32_t seed, Workload mode)
        : program(std::move(instructions)), random(seed), workload(mode), variables{{"x", 0}} {}

    const Instruction& nextInstruction(Tick remaining, const std::string& name) {
        for (;;) {
            advanceLoops();
            const Instruction* instruction;
            if (!loops.empty()) {
                LoopFrame& frame = loops.back();
                instruction = &frame.loop->body[frame.next++];
            } else if (program) {
                instruction = &(*program)[nextRoot++];
            } else {
                if (workload == Workload::alternating) {
                    // The initial x=0 is process state, not an extra instruction.
                    // Preserve the exact configured PRINT, ADD, PRINT, ADD... length.
                    if (nextRoot++ % 2 == 0) {
                        generated = Instruction{Print{"Value from: ", std::string{"x"}}};
                    } else {
                        const auto increment = static_cast<std::uint16_t>(
                            std::uniform_int_distribution<int>(1, 10)(random));
                        generated = Instruction{Add{"x", std::string{"x"}, increment}};
                    }
                } else {
                    generated = generateInstruction(random, name, remaining);
                }
                instruction = &*generated;
            }
            const auto* loop = std::get_if<For>(&instruction->operation);
            if (!loop) {
                return *instruction;
            }
            if (loop->repeats != 0 && (!program || countInstructions(loop->body) != 0)) {
                loops.push_back({loop});
            }
        }
    }

    void advanceLoops() {
        while (!loops.empty()) {
            LoopFrame& frame = loops.back();
            if (frame.next < frame.loop->body.size()) {
                return;
            }
            if (++frame.iteration < frame.loop->repeats) {
                frame.next = 0;
                return;
            }
            loops.pop_back();
        }
    }

    std::uint16_t read(const Operand& operand) {
        if (const auto* literal = std::get_if<std::uint16_t>(&operand)) {
            return *literal;
        }
        return variables[std::get<std::string>(operand)];
    }

    void store(const std::string& target, int value) {
        variables[target] = static_cast<std::uint16_t>(
            std::clamp(value, 0, static_cast<int>(std::numeric_limits<std::uint16_t>::max())));
    }

    std::optional<Program> program;
    std::size_t nextRoot = 0;
    std::minstd_rand random;
    Workload workload;
    std::optional<Instruction> generated;
    std::vector<LoopFrame> loops;
    std::unordered_map<std::string, std::uint16_t> variables;
    Tick delayLeft = 0;
};

} // namespace

Config readConfig(const std::filesystem::path& file, std::ostream& warnings) {
    std::ifstream input(file);
    if (!input) {
        throw std::runtime_error("Could not open " + file.string() + ".");
    }

    std::unordered_map<std::string, std::string> values;
    std::string line;
    while (std::getline(input, line)) {
        std::istringstream row(line);
        std::string key, value, extra;
        if (!(row >> key)) {
            continue;
        }
        if (key == "delays-per-exec") {
            key = "delay-per-exec";
        }
        if (!(row >> value) || row >> extra || !values.emplace(key, value).second) {
            throw std::runtime_error("Expected one value for " + key + ".");
        }
    }
    if (input.bad()) {
        throw std::runtime_error("Could not read " + file.string() + ".");
    }

    const auto take = [&values](const std::string& key) {
        auto entry = values.extract(key);
        if (entry.empty()) {
            throw std::runtime_error("Missing " + key + " in config.txt.");
        }
        auto value = std::move(entry.mapped());
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            return value.substr(1, value.size() - 2);
        }
        return value;
    };
    const auto setting = [&take, &warnings](const std::string& key, Tick low, Tick high) {
        return number(take(key), key, low, high, warnings);
    };
    Config config;
    config.cores = static_cast<std::size_t>(setting("num-cpu", 1, 128));
    auto scheduler = take("scheduler");
    std::transform(scheduler.begin(), scheduler.end(), scheduler.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    if (scheduler == "fcfs") {
        config.algorithm = Algorithm::fcfs;
    } else if (scheduler == "rr") {
        config.algorithm = Algorithm::rr;
    } else {
        throw std::runtime_error("scheduler must be fcfs or rr.");
    }
    config.quantum = setting("quantum-cycles", 1, maxSetting);
    config.batchFrequency = setting("batch-process-freq", 1, maxSetting);
    config.minInstructions = setting("min-ins", 1, maxSetting);
    config.maxInstructions = setting("max-ins", 1, maxSetting);
    config.executionDelay = setting("delay-per-exec", 0, maxSetting);
    if (config.minInstructions > config.maxInstructions) {
        throw std::runtime_error("min-ins must be less than or equal to max-ins.");
    }
    if (!values.empty()) {
        throw std::runtime_error("Unknown setting " + values.begin()->first + ".");
    }
    return config;
}

struct Scheduler::Process {
    ProcessSummary summary;
    std::unique_ptr<Execution> execution;
    std::vector<LogEntry> logs;
};

Scheduler::Scheduler(Config config, std::uint32_t seed)
    : config_(config), random_(seed), cores_(config.cores) {}

Scheduler::~Scheduler() = default;

ProcessId Scheduler::create(std::string name) {
    return create(std::move(name), std::nullopt);
}

ProcessId Scheduler::create(std::string name, Program program) {
    return create(std::move(name), std::optional<Program>{std::move(program)});
}

ProcessId Scheduler::create(std::string name, std::optional<Program> program) {
    if (names_.find(name) != names_.end()) {
        throw std::invalid_argument("Process " + name + " already exists.");
    }
    const ProcessId id = processes_.size();
    const Tick total = program
        ? countInstructions(*program)
        : std::uniform_int_distribution<Tick>(config_.minInstructions,
                                               config_.maxInstructions)(random_);
    if (total == 0) {
        throw std::invalid_argument("A process must have at least one instruction.");
    }
    auto execution = std::make_unique<Execution>(std::move(program), random_(), config_.workload);
    processes_.push_back({{id, std::move(name), std::chrono::system_clock::now(), State::ready,
                           std::nullopt, 0, total}, std::move(execution), {}});
    names_.emplace(processes_.back().summary.name, id);
    enqueue(processes_.back());
    return id;
}

std::optional<ProcessId> Scheduler::findLive(const std::string& name) const {
    const auto it = names_.find(name);
    if (it == names_.end() || processes_[it->second].summary.state == State::finished) {
        return std::nullopt;
    }
    return it->second;
}

ProcessScreen Scheduler::screen(ProcessId id, std::size_t firstLog) const {
    const auto& process = processes_.at(id);
    const auto begin = process.logs.begin() + static_cast<std::ptrdiff_t>(
        std::min(firstLog, process.logs.size()));
    return {process.summary, {begin, process.logs.end()}};
}

Utilization Scheduler::utilization() const {
    Utilization snapshot{cores_.size(), 0, {}, {}, {}, {}};
    for (const auto& core : cores_) {
        if (core.process) {
            snapshot.running.push_back(processes_[*core.process].summary);
        }
    }
    snapshot.used = snapshot.running.size();
    for (const auto& process : processes_) {
        if (process.summary.state == State::finished) {
            snapshot.finished.push_back(process.summary);
        } else if (process.summary.state == State::ready) {
            snapshot.ready.push_back(process.summary);
        } else if (process.summary.state == State::sleeping) {
            snapshot.sleeping.push_back(process.summary);
        }
    }
    return snapshot;
}

void Scheduler::startBatch() {
    if (!nextArrival_) {
        nextArrival_ = tick_ + config_.batchFrequency;
    }
}

void Scheduler::stopBatch() {
    nextArrival_.reset();
}

void Scheduler::enqueue(Process& process) {
    process.summary.state = State::ready;
    process.summary.core.reset();
    ready_.push_back(process.summary.id);
}

void Scheduler::finish(Process& process) {
    process.summary.state = State::finished;
    process.summary.core.reset();
    process.execution.reset();
}

void Scheduler::execute(Core& core) {
    Process& process = processes_[*core.process];
    auto& summary = process.summary;
    Execution& execution = *process.execution;
    ++core.usedTicks;
    if (execution.delayLeft != 0) {
        --execution.delayLeft;
        return;
    }

    const auto& instruction = execution.nextInstruction(summary.total - summary.completed,
                                                         summary.name);
    const auto& operation = instruction.operation;
    std::optional<Tick> sleepTicks;
    if (const auto* print = std::get_if<Print>(&operation)) {
        std::string message = print->text;
        if (print->variable) {
            message += std::to_string(execution.variables[*print->variable]);
        }
        process.logs.push_back({std::chrono::system_clock::now(), *summary.core,
                                std::move(message)});
    } else if (const auto* declare = std::get_if<Declare>(&operation)) {
        execution.variables[declare->variable] = declare->value;
    } else if (const auto* add = std::get_if<Add>(&operation)) {
        execution.store(add->target, execution.read(add->left) + execution.read(add->right));
    } else if (const auto* subtract = std::get_if<Subtract>(&operation)) {
        execution.store(subtract->target,
                        execution.read(subtract->left) - execution.read(subtract->right));
    } else if (const auto* sleep = std::get_if<Sleep>(&operation)) {
        sleepTicks = sleep->ticks;
    }
    ++summary.completed;
    execution.delayLeft = config_.executionDelay;
    if (sleepTicks) {
        summary.state = State::sleeping;
        summary.core.reset();
        sleeping_.emplace(tick_ + *sleepTicks + 1, summary.id);
        core = {};
    } else if (summary.completed == summary.total) {
        finish(process);
        core = {};
    }
}

void Scheduler::generateArrival() {
    std::string name;
    do {
        const auto number = std::to_string(++batchNumber_);
        name = (batchNumber_ < 10 ? "p0" : "p") + number;
    } while (names_.find(name) != names_.end());
    create(std::move(name));
    *nextArrival_ += config_.batchFrequency;
}

void Scheduler::dispatch() {
    for (std::size_t i = 0; i < cores_.size() && !ready_.empty(); ++i) {
        auto& core = cores_[i];
        if (!core.process) {
            core.process = ready_.front();
            ready_.pop_front();
            auto& summary = processes_[*core.process].summary;
            summary.state = State::running;
            summary.core = i;
        }
    }
}

void Scheduler::step() {
    ++tick_;
    while (!sleeping_.empty() && sleeping_.top().first <= tick_) {
        auto& process = processes_[sleeping_.top().second];
        sleeping_.pop();
        if (process.summary.completed == process.summary.total) {
            finish(process);
        } else {
            enqueue(process);
        }
    }

    if (config_.algorithm == Algorithm::rr) {
        for (auto& core : cores_) {
            if (core.process && core.usedTicks == config_.quantum) {
                enqueue(processes_[*core.process]);
                core = {};
            }
        }
    }

    dispatch();
    for (auto& core : cores_) {
        if (core.process) {
            execute(core);
        }
    }
    // A replacement can occupy a vacated core now but executes on the next tick.
    dispatch();

    if (nextArrival_ && tick_ == *nextArrival_) {
        generateArrival();
    }
}

} // namespace csopesy
