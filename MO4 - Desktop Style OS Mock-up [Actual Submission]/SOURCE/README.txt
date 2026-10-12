======================================

MO4 - Desktop Style OS Mock-up [Actual Submission]

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
- Git (CMake downloads GLFW and Dear ImGui during the first configuration)
- A C++17 MinGW compiler, such as the MSYS2 UCRT64 toolchain
- Visual Studio

======================================

Option 1: Running via Visual Studio.
- Open Visual Studio and select Open a local folder, or File → Open → Folder.
- Choose this folder: \SOURCE
- Choose an x64 Debug configuration and select  csopesy_os.exe  as the startup target.
- Ctrl+F5 to run without debugging.

======================================

Option 2: Running via Command Line.

Build from the project root:

  cmake -S . -B build -G "MinGW Makefiles"
  cmake --build build

Run:

  .\build\csopesy_os.exe

======================================

The program opens as a maximized desktop-style OS mockup. Use the Files,
Settings, and Task Manager buttons on the taskbar to open its UI screens.
Close the application using the red PWR button in the upper-right corner.