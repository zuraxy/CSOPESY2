#pragma once

#include <chrono>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace csopesy {

using Tick = std::uint64_t;
using ProcessId = std::size_t;
using Time = std::chrono::system_clock::time_point;

enum class Algorithm { fcfs, rr };
enum class State { ready, running, sleeping, finished };

struct Config {
    std::size_t cores = 4;
    Algorithm algorithm = Algorithm::rr;
    Tick quantum = 5;
    Tick batchFrequency = 1;
    Tick minInstructions = 1000;
    Tick maxInstructions = 2000;
    Tick executionDelay = 0;
};

Config readConfig(const std::filesystem::path& file, std::ostream& warnings);

using Operand = std::variant<std::uint16_t, std::string>;
struct Print {
    std::string text;
    std::optional<std::string> variable;
};
struct Declare { std::string variable; std::uint16_t value; };
struct Add { std::string target; Operand left; Operand right; };
struct Subtract { std::string target; Operand left; Operand right; };
struct Sleep { std::uint8_t ticks; };
struct Instruction;
struct For { std::vector<Instruction> body; std::uint32_t repeats; };
struct Instruction {
    std::variant<Print, Declare, Add, Subtract, Sleep, For> operation;
};
using Program = std::vector<Instruction>;

struct LogEntry {
    Time time;
    std::size_t core;
    std::string message;
};

struct ProcessSummary {
    ProcessId id;
    std::string name;
    Time created;
    State state;
    std::optional<std::size_t> core;
    Tick completed;
    Tick total;
};

struct ProcessScreen {
    ProcessSummary process;
    std::vector<LogEntry> logs;
};

struct Utilization {
    std::size_t cores;
    std::size_t used;
    std::vector<ProcessSummary> running;
    std::vector<ProcessSummary> finished;
};

// The caller serializes access when the scheduler is shared with the console.
class Scheduler {
public:
    explicit Scheduler(Config config, std::uint32_t seed = std::random_device{}());
    ~Scheduler();

    ProcessId create(std::string name);
    ProcessId create(std::string name, Program program);
    std::optional<ProcessId> findLive(const std::string& name) const;
    ProcessScreen screen(ProcessId id) const;
    Utilization utilization() const;

    void startBatch();
    void stopBatch();
    void step();

private:
    struct Process;
    struct Core {
        std::optional<ProcessId> process;
        Tick usedTicks = 0;
    };
    using Wakeup = std::pair<Tick, ProcessId>;

    ProcessId create(std::string name, std::optional<Program> program);
    void enqueue(Process& process);
    void finish(Process& process);
    void dispatch();
    void execute(Core& core);
    void generateArrival();

    Config config_;
    std::mt19937 random_;
    Tick tick_ = 0;
    Tick batchNumber_ = 0;
    std::optional<Tick> nextArrival_;
    std::vector<Core> cores_;
    std::vector<Process> processes_;
    std::unordered_map<std::string, ProcessId> names_;
    std::deque<ProcessId> ready_;
    std::priority_queue<Wakeup, std::vector<Wakeup>, std::greater<Wakeup>> sleeping_;
};

} // namespace csopesy
