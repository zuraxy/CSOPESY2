CSOPESY Machine Project
======================

Developers:
    Karl Matthew Dela Cruz
    Bon Aquino
    Dencel Angelo Pineda
    Jose Miguel Espinosa
Entry point: main() in main.cpp

Build and run
-------------
Use a C++17 compiler with thread support, CMake 3.16 or newer, and Ninja.
On Windows, the project was built with GCC from MSYS2 UCRT64.

From the source directory:

    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ctest --test-dir build --output-on-failure
    .\build\csopesy.exe

On Linux or macOS, run ./build/csopesy instead.
The emulator reads config.txt from its working directory. Launch from the
source directory to use the config.txt alongside this README. The utilization
report is saved to csopesy-log.txt in that same working directory.

Commands
--------
initialize        Load config.txt and start the simulation clock.
screen -s NAME    Create a process and enter its screen.
screen -r NAME    Enter an existing unfinished process's screen.
screen -ls        Show core usage and running and finished process summaries.
scheduler-start   Generate one process per configured batch interval.
scheduler-stop    Stop arrivals while queued and running work continues.
report-util       Save a utilization snapshot to csopesy-log.txt.
exit              Exit the emulator from the main menu.

Inside a process screen, process-smi refreshes progress and PRINT logs.
Inside a process screen, exit returns to the main menu.
Batch processes have names such as p01 and p02. Use screen -r to open them.

Quiz setup
----------
Build once. For each test case, set the required values in config.txt, start
the same executable, then type initialize. Each invocation has its own tick
counter and starts with an empty process list.

The included config.txt matches the handout's sample. The scheduler value
accepts fcfs or rr regardless of letter case. Values may use double quotes.
Both delay-per-exec and delays-per-exec are accepted spellings. Integer values
outside their individual ranges are clamped with a warning. The maximum core
count is 128, and the maximum cycle or instruction setting is 2^32.

For a rehearsal, enter initialize, screen -s demo, process-smi, and exit.
Run scheduler-start briefly, then scheduler-stop, screen -ls, and report-util.
After demo finishes, screen -r demo prints the required not-found message.

Simulation details
------------------
A background thread advances the shared clock approximately every 5 ms. Each
tick wakes due sleepers, requeues expired RR slices, and dispatches and
advances the cores in index order. A core released by sleep or completion can
take another ready process, which begins execution on the next tick. A due
batch arrival joins the queue at the end of the tick. Equal wake times are
ordered by process ID.

The RR quantum counts occupied ticks, including busy-wait ticks. A busy-wait
delay preserves the core assignment unless RR preempts the process. Remaining
delay survives preemption. SLEEP X releases the core and blocks the process
for the next X full ticks. SLEEP 0 yields until the next dispatch cycle.

Progress counts executed instructions, including repeated instructions inside
FOR bodies. Loop headers use no separate CPU tick. Generated programs choose
uniformly among the instruction types allowed at each nesting level and fit
the expanded body into the remaining instruction budget. Process variables
are released at completion. Logs and summary information are retained.
