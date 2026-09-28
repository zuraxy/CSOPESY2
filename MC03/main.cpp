#include "AppState.h"
#include "CommandInterpreter.h"
#include "ConsoleUI.h"
#include "Marquee.h"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <windows.h>
#include <mmsystem.h>

int main() {
    AppState app;
    int w, h;
    getConsoleSize(w, h);
    app.consoleW = (DESIRED_W < w - 1) ? DESIRED_W : (w - 1); if (app.consoleW < 20) app.consoleW = 20;
    app.consoleH = (DESIRED_H < h)     ? DESIRED_H : h;       if (app.consoleH < 18) app.consoleH = 18;
    app.marqueeTop    = HEADER_ROWS;
    app.marqueeBottom = app.consoleH - OUTPUT_ROWS - 3;
    app.x = 1;
    app.y = HEADER_ROWS + 1;
    app.vx = 1;
    app.vy = 1;
    app.running = true;
    app.marqueeRunning = true;
    app.measuredFps = 0.0;
    app.refreshMs = 16;
    app.art = BIG_CSOPESY;
    app.output.push_back("Type 'help' for commands or 'exit' to quit.");

    // Keep redirected input useful for scripted command checks.
    if (!isInteractiveConsole()) {
        std::string line;
        while (app.running && std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            app.currentInput = line;
            submitCommand(app);
            for (const std::string& message : app.output) std::cout << message << '\n';
            app.currentInput.clear();
        }
        return 0;
    }

    timeBeginPeriod(1);
    std::system("cls");
    setCursorVisible(false);

    typedef std::chrono::steady_clock Clock;
    Clock::time_point lastPoll = Clock::now();
    Clock::time_point lastDraw = Clock::now();
    Clock::time_point lastMove = Clock::now();
    Clock::time_point fpsMark  = Clock::now();
    int frames = 0;

    while (app.running) {
        Clock::time_point now = Clock::now();

        if (std::chrono::duration<double, std::milli>(now - lastPoll).count() >= POLL_INTERVAL_MS) {
            pollInput(app);
            lastPoll = now;
        }

        if (std::chrono::duration<double, std::milli>(now - lastDraw).count() >= app.refreshMs) {
            double dt = std::chrono::duration<double>(now - lastMove).count();
            lastMove = now;
            if (app.marqueeRunning) updateMotion(app, dt);

            frames++;
            double since = std::chrono::duration<double>(now - fpsMark).count();
            if (since >= 0.5) {
                app.measuredFps = frames / since;
                frames = 0;
                fpsMark = now;
            }

            drawFrame(buildFrame(app));
            lastDraw = now;
        }

        if (BASE_SLEEP_MS > 0) Sleep((DWORD)BASE_SLEEP_MS); else Sleep(0);
    }

    setCursorVisible(true);
    setCursorPosition(0, app.consoleH);
    std::cout << std::endl;
    timeEndPeriod(1);
    return 0;
}
