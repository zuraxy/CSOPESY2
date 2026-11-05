#include "console.hpp"

// Separate runner requested for Detailed-Specs.pdf, Question 6.
//
// The stated rates cannot drain in 30 seconds with an unchanged wall-clock tick:
// 10 seconds * 1 arrival/tick * 100 instructions / 8 cores = 125 seconds of work.
// After the first 10 seconds, approximately 115 seconds of work remain.
//
// This runner explicitly changes the WALL-CLOCK duration of a logical CPU tick
// from 5 ms to 1 ms after scheduler-stop (a 5x drain speed). It still executes at
// most one instruction per core per logical tick, preserves RR quantum=1, and
// never changes completed counts, utilization, arrival frequency, or logs.
// scheduler-start/scheduler-test restores 5 ms ticks for the next batch.
//
// The regular csopesy executable uses 5 ms ticks throughout. This accommodation
// is opt-in by launching this executable; it is not selected from config values.
// Use: case6_runner --config configs/case-6.txt

int main(int argc, char** argv) {
    using namespace std::chrono_literals;
    return csopesy::runConsole(argc, argv, {5ms, 1ms,
        "Question 6 runner: 5 ms/tick while generating; 1 ms/tick after scheduler-stop."});
}
