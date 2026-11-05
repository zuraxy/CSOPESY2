======================================
ACTUAL Submission - MO1 - Process Multiplexer and CLI
======================================

Authors:
- Karl Matthew Dela Cruz
- Jose Miguel Espinosa
- Bon Windel Aquino
- Dencel Angelo Pineda

Entry files:
- src/main.cpp: regular emulator
- runners/case6_runner.cpp: separate Question 6 timing runner

Requirements:
- CMake 3.16 or newer
- C++17 compiler with thread support (MSVC or MinGW on Windows)
- Ninja for the Ninja commands below
- Python 3 is optional, for CLI/elapsed-time acceptance checks

Build and run from this SOURCE folder:

  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build
  .\build\csopesy.exe

The executable can be launched from another working directory. By default it
reads config.txt beside the actual executable. CMake copies config.txt and
configs/ into each target's output directory, including Debug/Release folders.
Keep the matching MinGW runtime DLLs available through PATH when using MinGW.

Visual Studio:
- Open this SOURCE folder in Visual Studio using Open a local folder.
- Select csopesy.exe as the startup target; run without debugging.
- For a Visual Studio multi-configuration command-line build:

  cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
  cmake --build build-msvc --config Debug
  .\build-msvc\Debug\csopesy.exe

Use the Visual Studio generator matching your installed version. On Linux/macOS
the executable names have no .exe extension.

Configuration and reports:
- initialize loads the configuration; all commands except exit are gated until
  initialization succeeds.
- --config <file> explicitly selects a configuration. Relative paths resolve
  from your working directory; the chosen absolute path is printed at startup.
- Edit the selected runtime config before starting the program. Recompilation is
  not needed to change scheduler parameters.
- report-util saves csopesy-log.txt beside the selected configuration file.
- The report includes Running, Finished, Ready, and Sleeping processes. Only
  processes assigned to cores count toward CPU utilization.
- Both delay-per-exec and delays-per-exec are accepted.

Commands:
- initialize, exit
- screen -s <name>, screen -r <name>
- screen -ls and screen-ls (both spellings)
- scheduler-start and scheduler-test (both spellings)
- scheduler-stop, report-util
- Inside a process screen: process-smi, exit
- PRINT logs stream automatically while attached. process-smi also reprints the
  current process details and retained logs. Leaving a screen stops its output.

Detailed-Specs.pdf Questions 4-8:
Read TEST_CASES.md for the exact sequences, selected X/Y values, and limitations.
Run from this SOURCE folder:

  .\build\csopesy.exe --config .\build\configs\case-4.txt
  .\build\csopesy.exe --config .\build\configs\case-5.txt --workload alternating
  .\build\case6_runner.exe --config .\build\configs\case-6.txt
  .\build\csopesy.exe --config .\build\configs\case-7.txt
  .\build\csopesy.exe --config .\build\configs\case-8.txt

--workload alternating is the explicit Question 5 preparation: x starts at zero
in every process, with exactly PRINT("Value from: " + x), ADD(x,x,random[1,10]),
PRINT, ADD, ... for the configured instruction count. The usual workload is
random and supports all six instruction types. Arithmetic saturates at 0/65535.

Question 6 has a documented capacity/timing conflict. The SEPARATE runner
changes logical tick duration from 5 ms to 1 ms after scheduler-stop, restoring
5 ms on the next scheduler-start. The regular emulator remains 5 ms throughout.
The runner prints this policy at startup and documents its rationale inline.
Every logical tick still executes at most one instruction per core.

Verification:

  ctest --test-dir build --output-on-failure

The C++ unit suite always runs. CMake adds the six CLI integration checks when
it finds Python 3. Run actual waiting intervals and save transcripts/results:

  python tests\acceptance.py --app build\csopesy.exe --runner build\case6_runner.exe --output build\acceptance-results

Choose a new output directory for another acceptance run. These are genuine
execution transcripts and a JSON result summary, not recorded assessment videos.
