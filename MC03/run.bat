@echo off
setlocal

cd /d "%~dp0"

where g++ >nul 2>nul
if errorlevel 1 (
    echo Error: g++ was not found in PATH.
    echo Install MinGW-w64, then reopen Command Prompt and try again.
    exit /b 1
)

echo Compiling marquee console...
g++ -std=c++17 -Wall -Wextra -pedantic main.cpp CommandInterpreter.cpp Marquee.cpp ConsoleUI.cpp -lwinmm -o csopesy_marquee.exe
if errorlevel 1 (
    echo Compilation failed.
    exit /b 1
)

echo Starting program...
echo.
csopesy_marquee.exe

endlocal
