CSOPESY OS - Dear ImGui Desktop Mockup
======================================

Authors:
- Karl Matthew Dela Cruz
- Jose Miguel Espinosa
- Bon Windel Aquino
- Dencel Angelo Pineda

Entry file:
src/main.cpp

Requirements:
- Windows
- CMake 3.16 or newer
- Git (CMake downloads GLFW and Dear ImGui during the first configuration)
- A C++17 MinGW compiler, such as the MSYS2 UCRT64 toolchain

Build from the project root:

  cmake -S . -B build -G "MinGW Makefiles"
  cmake --build build

Run:

  .\build\csopesy_os.exe

The program opens as a maximized desktop-style OS mockup. Use the Files,
Settings, and Task Manager buttons on the taskbar to open its UI screens.
Close the application using the red PWR button in the upper-right corner.

The separate command-line MCO-1 process emulator is located in emulator/.
