#include "scheduler.hpp"

#include <iostream>
#include <stdexcept>

using namespace csopesy;

namespace {

void expect(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

Config config(Algorithm algorithm, std::size_t cores = 1, Tick quantum = 2) {
    Config result;
    result.algorithm = algorithm;
    result.cores = cores;
    result.quantum = quantum;
    return result;
}

Instruction print(std::string text = "test", std::optional<std::string> variable = {}) {
    return {Print{std::move(text), std::move(variable)}};
}

void checkOrder(Algorithm algorithm, const std::string& expected) {
    Scheduler scheduler(config(algorithm), 1);
    const auto a = scheduler.create("A", Program(3, print()));
    const auto b = scheduler.create("B", Program(2, print()));
    Tick aProgress = 0;
    std::string actual;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        scheduler.step();
        const auto progress = scheduler.screen(a).process.completed;
        actual += progress > aProgress ? 'A' : 'B';
        aProgress = progress;
    }
    expect(actual == expected, "Unexpected schedule: " + actual);
    expect(scheduler.screen(b).process.completed == 2, "B did not finish");
    expect(scheduler.utilization().finished.size() == 2, "Missing completed processes");
}

void checkMulticore() {
    Scheduler scheduler(config(Algorithm::rr, 128, 1), 1);
    for (int i = 0; i < 129; ++i) {
        scheduler.create("job" + std::to_string(i), Program(2, print()));
    }
    scheduler.step();
    const auto report = scheduler.utilization();
    expect(report.used == 128 && report.running.size() == 128, "Cores not fully assigned");
    for (std::size_t core = 0; core < 128; ++core) {
        const auto screen = scheduler.screen(report.running[core].id);
        expect(screen.logs.size() == 1 && screen.logs[0].core == core,
               "Process did not run once on its assigned core");
    }
    expect(scheduler.screen(128).process.state == State::ready, "Overflow process not queued");
    scheduler.step();
    expect(scheduler.screen(128).process.completed == 1, "RR did not dispatch queued process");
}

void checkSleep() {
    Scheduler scheduler(config(Algorithm::fcfs), 1);
    const auto a = scheduler.create("A", Program{{Sleep{2}}, print()});
    const auto b = scheduler.create("B", Program(2, print()));
    scheduler.step();
    expect(scheduler.screen(a).process.state == State::sleeping, "SLEEP did not block");
    const auto report = scheduler.utilization();
    expect(report.used == 1 && report.running.front().id == b
               && report.running.front().completed == 0,
           "SLEEP did not free its core for B without executing twice in one tick");
    scheduler.step();
    expect(scheduler.screen(b).process.completed == 1, "Core did not dispatch B");
    scheduler.step();
    expect(scheduler.screen(a).process.state == State::sleeping, "A woke too soon");
    scheduler.step();
    expect(!scheduler.findLive("A") && scheduler.screen(a).logs.size() == 1,
           "A did not wake, finish, and retain its log");

    const auto yielding = scheduler.create("yielding", Program{{Sleep{0}}, print()});
    const auto next = scheduler.create("next", Program{print()});
    scheduler.step();
    scheduler.step();
    expect(scheduler.screen(next).process.state == State::finished
               && scheduler.findLive("yielding") == yielding,
           "SLEEP 0 did not yield to a process already in the queue");
}

void checkDelay() {
    auto settings = config(Algorithm::rr, 1, 2);
    settings.executionDelay = 2;
    Scheduler scheduler(settings, 1);
    const auto a = scheduler.create("A", Program(2, print()));
    const auto b = scheduler.create("B", Program{print()});
    scheduler.step();
    scheduler.step();
    expect(scheduler.screen(a).process.completed == 1 && scheduler.utilization().used == 1,
           "Busy wait executed an instruction or released the core");
    scheduler.step();
    expect(scheduler.screen(b).process.completed == 1, "Busy wait did not count toward quantum");
    scheduler.step();
    expect(scheduler.screen(a).process.completed == 1, "Busy wait reset during preemption");
    scheduler.step();
    expect(scheduler.screen(a).process.completed == 2, "A did not resume after delay");
}

void checkInstructions() {
    Scheduler scheduler(config(Algorithm::rr, 1, 1), 1);
    const Instruction inner{For{{{Add{"x", std::string{"x"}, std::uint16_t{4}}},
                                 {Sleep{1}}, print("x=", "x")}, 2}};
    const Instruction middle{For{{inner}, 1}};
    const Instruction outer{For{{middle}, 2}};
    const auto id = scheduler.create("variables", Program{
        {Declare{"x", 65530}},
        outer,
        {Subtract{"y", std::string{"undeclared"}, std::uint16_t{1}}},
        print("y=", "y"),
    });
    scheduler.create("competitor", Program(40, print()));
    scheduler.step();
    scheduler.step();
    scheduler.step();
    const auto progress = scheduler.screen(id).process;
    expect(progress.completed == 2 && progress.total == 15,
           "FOR headers consumed a tick or contributed to instruction progress");
    for (int i = 0; i < 80 && scheduler.findLive("variables"); ++i) {
        scheduler.step();
    }
    const auto screen = scheduler.screen(id);
    expect(screen.process.state == State::finished && screen.process.completed == 15,
           "Nested loop did not complete and resume correctly");
    std::vector<std::string> messages;
    for (const auto& entry : screen.logs) {
        messages.push_back(entry.message);
    }
    expect(messages == std::vector<std::string>{"x=65534", "x=65535", "x=65535", "x=65535", "y=0"},
           "Loop, sleep, variable or clamping behavior incorrect");
}

void checkGeneratedLengths() {
    auto settings = config(Algorithm::rr, 16);
    settings.minInstructions = 1;
    settings.maxInstructions = 16;
    Scheduler scheduler(settings, 1);
    for (int i = 0; i < 128; ++i) {
        const auto id = scheduler.create("generated" + std::to_string(i));
        const auto screen = scheduler.screen(id);
        expect(screen.process.total >= 1 && screen.process.total <= 16,
               "Generated process length was outside the configured range");
    }
    for (int i = 0; i < 250; ++i) {
        scheduler.step();
    }
    const auto report = scheduler.utilization();
    expect(report.finished.size() == 128, "Generated processes did not finish");
    for (const auto& process : report.finished) {
        expect(process.completed == process.total, "Generated progress did not match total");
    }
}

void checkBatch() {
    auto settings = config(Algorithm::rr);
    settings.batchFrequency = 2;
    settings.minInstructions = settings.maxInstructions = 1;
    Scheduler scheduler(settings, 1);
    scheduler.startBatch();
    scheduler.step();
    scheduler.startBatch();
    expect(!scheduler.findLive("p01"), "Batch generated before its scheduled tick");
    scheduler.step();
    const auto id = scheduler.findLive("p01");
    expect(id && scheduler.screen(*id).process.state == State::ready,
           "Batch arrival did not enter ready queue at end of tick");
    scheduler.stopBatch();
    for (int i = 0; i < 20; ++i) {
        scheduler.step();
    }
    expect(scheduler.screen(*id).process.state == State::finished && !scheduler.findLive("p02"),
           "Batch stop did not preserve existing work or prevent new arrivals");
    scheduler.startBatch();
    scheduler.step();
    scheduler.step();
    expect(scheduler.findLive("p02").has_value(), "Batch restart did not continue naming");
}

void checkAlternatingWorkload() {
    auto settings = config(Algorithm::rr, 1, 20);
    settings.minInstructions = settings.maxInstructions = 100000;
    settings.workload = Workload::alternating;
    for (int run = 0; run < 2; ++run) {
        Scheduler scheduler(settings, static_cast<std::uint32_t>(run + 1));
        const auto id = scheduler.create("proc-01");
        int previous = 0;
        std::size_t shown = 0;
        for (int tick = 1; tick <= 201; ++tick) {
            scheduler.step();
            const auto screen = scheduler.screen(id, shown);
            expect(screen.process.completed == static_cast<Tick>(tick),
                   "Alternating workload inserted an extra instruction or SLEEP");
            expect(screen.logs.size() == static_cast<std::size_t>(tick % 2),
                   "Workload must alternate PRINT then ADD");
            if (!screen.logs.empty()) {
                const auto& text = screen.logs.front().message;
                expect(text.rfind("Value from: ", 0) == 0, "Incorrect variable PRINT text");
                const int value = std::stoi(text.substr(12));
                expect(shown == 0 ? value == 0 : value - previous >= 1 && value - previous <= 10,
                       "x did not start at zero or ADD was outside [1,10]");
                previous = value;
                ++shown;
            }
        }
        for (int tick = 201; tick < 100000; ++tick) {
            scheduler.step();
        }
        const auto screen = scheduler.screen(id);
        expect(screen.process.state == State::finished && screen.process.total == 100000
                   && screen.logs.size() == 50000 && screen.logs.back().message == "Value from: 65535",
               "Alternating workload length or uint16 saturation is incorrect");
        expect(scheduler.screen(id, screen.logs.size()).logs.empty(), "Live log cursor repeats logs");
    }
}

void checkReportedStates() {
    Scheduler scheduler(config(Algorithm::fcfs), 1);
    const auto sleeper = scheduler.create("sleeper", Program{{Sleep{3}}, print()});
    scheduler.create("runner", Program(10, print()));
    scheduler.create("queued", Program(10, print()));
    scheduler.step();
    auto report = scheduler.utilization();
    expect(report.used == 1 && report.running.size() == 1 && report.ready.size() == 1
               && report.sleeping.size() == 1 && report.sleeping.front().id == sleeper,
           "Report must retain ready/sleeping work without inflating cores used");
}

void checkDetailedBatchCases() {
    auto settings = config(Algorithm::rr, 4, 5);
    settings.minInstructions = 1000;
    settings.maxInstructions = 2000;
    Scheduler busy(settings, 42);
    busy.startBatch();
    for (int tick = 0; tick < 1000; ++tick) busy.step(); // Question 4: 5 s at 5 ms/tick.
    expect(busy.utilization().used == 4 && busy.utilization().finished.empty(),
           "Question 4 did not retain full utilization");

    settings = config(Algorithm::rr, 8, 1);
    settings.minInstructions = settings.maxInstructions = 100;
    Scheduler drain(settings, 42);
    drain.startBatch();
    for (int tick = 0; tick < 2000; ++tick) drain.step(); // Generate for 10 seconds.
    drain.stopBatch();
    for (int tick = 0; tick < 6000; ++tick) drain.step(); // 30 s at the regular 5 ms rate.
    expect(!drain.utilization().ready.empty(), "Question 6 capacity-conflict regression changed");
    for (int tick = 6000; tick < 30000; ++tick) drain.step(); // Separate runner: 1 ms/tick.
    const auto drained = drain.utilization();
    expect(drained.used == 0 && drained.ready.empty() && drained.sleeping.empty()
               && drained.finished.size() == 2000,
           "Question 6 work did not genuinely drain at the separate runner's tick budget");

    settings = config(Algorithm::rr, 1, 10);
    settings.batchFrequency = 30;
    settings.minInstructions = 30;
    settings.maxInstructions = 45;
    settings.executionDelay = 1;
    Scheduler intermittent(settings, 42);
    intermittent.startBatch();
    bool idle = false, active = false;
    for (int tick = 0; tick < 200; ++tick) {
        intermittent.step();
        idle = idle || intermittent.utilization().used == 0;
        active = active || intermittent.utilization().used == 1;
    }
    expect(idle && active, "Question 7 did not expose both utilization states");

    settings = config(Algorithm::rr, 16, 5);
    settings.minInstructions = settings.maxInstructions = 5000;
    settings.executionDelay = 5;
    Scheduler longJobs(settings, 42);
    longJobs.startBatch();
    for (int tick = 1; tick <= 6000; ++tick) {
        longJobs.step();
        if (tick >= 2000 && tick % 1000 == 0) {
            const auto snapshot = longJobs.utilization();
            expect(snapshot.used == 16 && snapshot.finished.size() < 10,
                   "Question 8 snapshot is not fully occupied or has too many completions");
        }
    }
}

} // namespace

int main() {
    try {
        checkOrder(Algorithm::fcfs, "AAABB");
        checkOrder(Algorithm::rr, "AABBA");
        checkMulticore();
        checkSleep();
        checkDelay();
        checkInstructions();
        checkGeneratedLengths();
        checkBatch();
        checkAlternatingWorkload();
        checkReportedStates();
        checkDetailedBatchCases();
        std::cout << "All scheduler checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
