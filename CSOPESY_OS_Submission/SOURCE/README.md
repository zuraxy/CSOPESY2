# CSOPESY OS — Dear ImGui Desktop Mockup

A desktop-style operating-system mockup built with GLFW, OpenGL, and Dear
ImGui. It provides a full-window desktop, real-time clock, shutdown control,
taskbar, File Explorer, Settings, and a Windows-inspired Task Manager.

## Authors

- Karl Matthew Dela Cruz
- Jose Miguel Espinosa
- Bon Windel Aquino
- Dencel Angelo Pineda

## Entry point

The program's `main` function is in [`src/main.cpp`](src/main.cpp).

## Requirements

- Windows
- CMake 3.16 or newer
- Git, used by CMake to download GLFW and Dear ImGui on the first configuration
- A C++17 MinGW compiler, such as the MSYS2 UCRT64 toolchain

## Build and run

From the repository root:

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
.\build\csopesy_os.exe
```

The first configuration downloads GLFW 3.4 and Dear ImGui v1.91.8. Later
builds reuse the downloaded dependencies in `build/`.

Use the red **PWR** button in the upper-right corner to close the program.
The native window close button is intentionally ignored.

## Separate command-line emulator

The MCO-1 process-scheduling console project remains in `emulator/`. It can be
built and tested independently:

```powershell
cd emulator
mingw32-make all
mingw32-make test
```
