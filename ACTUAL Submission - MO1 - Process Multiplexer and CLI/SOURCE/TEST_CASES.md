# Detailed specification case guide

Source: Detailed-Specs.pdf supplied by the user, pages 1-5, numbered Questions
4-8. The original six-page MO1 specification continues to govern instruction,
arithmetic, screen, and scheduling semantics. The detailed file was inspected
visually because the X/Y inequalities are missing from text extraction.

The executable reports actual scheduler state. No case detects configuration
numbers to replace utilization, completion counts, process logs, or arrivals.
Question 5 is selected explicitly by --workload alternating. Question 6 uses
the separate runner requested by the user.

## Question 4

Launch csopesy with configs/case-4.txt. Parameters are the supplied 4 CPUs, RR,
quantum 5, arrivals every tick, 1000-2000 instructions, execution delay 0.

1. initialize
2. scheduler-start
3. Wait 5 seconds.
4. screen-ls

Acceptance: 100% utilization, four assigned cores, predominantly unfinished
work. Ready/Sleeping rows expose backlog separately from running-core occupancy.

## Question 5

Launch csopesy with configs/case-5.txt and --workload alternating. Parameters
are exactly 1 CPU, RR, quantum 20, arrival frequency 1, 100000 instructions,
delay 0. Do not start batch generation in this case.

Every created process owns x=0 immediately. Its generated instructions alternate
PRINT("Value from: " + x) and ADD(x,x,random integer 1-10); initialization does
not consume an instruction. Variables are independent per process/run.

1. initialize
2. screen -s proc-01
3. exit (return to the menu)
4. screen-ls
5. screen -r proc-01
6. Wait 15 seconds. PRINT output updates without typing process-smi.
7. exit (leave screen), then exit (close emulator).
8. Relaunch with the same options and repeat.

Acceptance: first retained PRINT is zero, subsequent values increase by 1-10
until uint16 saturation. The supplied 15-second observation remains well below
saturation at the regular tick rate. Restart produces a fresh process with x=0.

## Question 6

Launch case6_runner with configs/case-6.txt. Parameters are exactly 8 CPUs, RR,
quantum 1, arrival frequency 1, 100 instructions, delay 0.

1. initialize
2. scheduler-test
3. Wait 10 seconds.
4. scheduler-stop
5. Wait 30 seconds.
6. screen-ls

Acceptance: 0% utilization, all generated work actually finished, no ready or
sleeping work remaining.

Rationale for the separate runner: an unchanged tick rate yields roughly
10 * 100 / 8 = 125 seconds of service for work generated over 10 seconds.
About 115 seconds remain after generation stops. This exceeds the stated
30-second draining interval. The user explicitly requested a separate
implementation or runner with inline documentation for this conflict.

The separate runner changes wall-clock tick duration from 5 ms to 1 ms after
scheduler-stop. It preserves CPU-tick semantics: one instruction per core per
tick, quantum=1, actual execution of every instruction, genuine SLEEP wakeups,
and genuine logs. Starting another batch restores 5 ms ticks. The regular
csopesy executable never selects this policy automatically.

The policy and capacity calculation are documented in runners/case6_runner.cpp
and announced by the runner. Do not present the regular emulator as having
passed this timing target or omit the runner distinction from evidence.

## Question 7

Launch csopesy with configs/case-7.txt. The chosen allowed values are X=30
(batch-process-freq) and Y=1 (delay-per-exec). The visually verified allowed
ranges are 10 <= X <= 30 and 1 <= Y <= 5.

1. initialize
2. Enter scheduler-test and immediately enter screen-ls. For a reliable capture,
   paste these as two separate lines together. The next arrival is 30 ticks
   away (approximately 150 ms); the initial report shows genuine 0% utilization.
3. Continue screen-ls snapshots to observe an assigned core and genuine 100%.

Limitation in the supplied parameters: a 30-45-instruction process with busy
delay 1-5 generally requires more service than the 10-30-tick interarrival gap.
Sustained generation therefore does not guarantee recurring idle periods.
The tested idle observation is before the first arrival; SLEEP may also expose
short idle windows. Do not claim these ranges guarantee recurring idle periods
due to a sufficiently slow arrival rate.

## Question 8

Launch csopesy with configs/case-8.txt. Choose allowed delay X=5 (the supplied
range is 0 <= X <= 5). Other parameters are exactly 16 CPUs, RR, quantum 5,
arrival frequency 1, 5000 instructions.

1. initialize
2. scheduler-test
3. Wait 10 seconds.
4. screen-ls five times, with 5 seconds between reports.
5. scheduler-stop
6. report-util

Acceptance: all five snapshots have 100% utilization and 16 running cores;
the saved report is a fresh snapshot, retains unfinished/finished work, and
has fewer than 10 finished processes. Stopping arrivals does not stop execution.
Separate commands may show different instruction counts because work continues.

## Verification artifacts

C++ tests cover instruction semantics, FCFS/RR ordering, multicore dispatch,
delays/sleep, all process report states, alternating workload/restart/saturation,
and the logical tick budgets for Questions 4/6/7/8.

tests/cli_tests.py checks actual executables through input/output pipes,
including executable-relative configuration discovery from a different working
directory, both listing spellings, report location, live logs without input,
screen restrictions, and the explicitly separate runner policy.

tests/acceptance.py executes the actual 5/15/10+30/10+5-second intervals and
runs Questions 4-8 in independent processes. It writes each selected config,
transcript.txt, the Question 8 csopesy-log.txt, and results.json. It never
replaces real waiting intervals with simulated test time.

These automated checks do not create MP4 files or prove native terminal visual
layout. Record the required assessment videos separately. The existing PPT
predates this revision; update its code references/examples before using it
as the technical report for the revised implementation.
