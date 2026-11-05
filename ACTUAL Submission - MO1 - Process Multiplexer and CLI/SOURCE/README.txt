======================================

ACTUAL Submission - MO1 - Process Multiplexer and CLI

======================================

Authors:
- Karl Matthew Dela Cruz
- Jose Miguel Espinosa
- Bon Windel Aquino
- Dencel Angelo Pineda

======================================

Entry file:
src/main.cpp

======================================

Requirements:
- Windows
- CMake 3.16 or newer
- A C++17 compiler with thread support, such as Visual Studio's MSVC or the
  MSYS2 UCRT64 (MinGW) toolchain
- Visual Studio (for Option 1)

======================================

Option 1: Running via Visual Studio.
- Open Visual Studio and select Open a local folder, or File -> Open -> Folder.
- Choose this folder: \SOURCE
- Choose an x64 Debug configuration and select  csopesy.exe  as the startup target.
- Ctrl+F5 to run without debugging.

======================================

Option 2: Running via Command Line.

Build from the project root:

  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
  cmake --build build

Run from the build folder:

  .\build\csopesy.exe

On Linux or macOS, run ./build/csopesy instead.

======================================

Configuration:
- config.txt is copied next to the executable when the project is built.
  The program reads config.txt from the folder it is launched in.
- csopesy-log.txt (from report-util) is saved in the same folder.
