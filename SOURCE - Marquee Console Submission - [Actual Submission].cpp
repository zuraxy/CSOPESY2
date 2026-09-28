#include <cctype>
#include <chrono>
#include <conio.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

const double TARGET_FPS        = 165.0;
const int    POLL_INTERVAL_MS  = 1;
const int    BASE_SLEEP_MS     = 1;
const int    ANIM_STEP         = 10;        
const int    DEFAULT_REFRESH_MS =
    static_cast<int>(1000.0 / TARGET_FPS + 0.5);

const int DESIRED_W   = 90;
const int DESIRED_H   = 41;
const int HEADER_ROWS = 13;
const int OUTPUT_ROWS = 7;

struct AppState {
    int consoleW = 0;
    int consoleH = 0;
    int marqueeTop = 0;
    int marqueeBottom = 0;
    int x = 0;
    int y = 0;
    int vx = 0;
    int vy = 0;
    std::string currentInput;
    std::vector<std::string> output;
    std::vector<std::string> art;
    bool running = false;
    bool marqueeRunning = false;
    double measuredFps = 0.0;
    int refreshMs = 0;
};

const std::vector<std::string> BIG_CSOPESY = {
    R"(  _____  _____  ____  _____  ______  _______     __)",
    R"( / ____|/ ____|/ __ \|  __ \|  ____|/ ____\ \   / /)",
    R"(| |    | (___ | |  | | |__) | |__  | (___  \ \_/ /)",
    R"(| |     \___ \| |  | |  ___/|  __|  \___ \  \   /)",
    R"(| |____ ____) | |__| | |    | |____ ____) |  | |)",
    R"( \_____|_____/ \____/|_|    |______|_____/   |_|)"
};

int logoWidth(const AppState& app) {
    int width = 0;
    for (const std::string& line : app.art) {
        if (static_cast<int>(line.size()) > width) {
            width = static_cast<int>(line.size());
        }
    }
    return width;
}

void updateMotion(AppState& app) {
    const int artW = logoWidth(app);
    const int artH = static_cast<int>(app.art.size());
    const int maxX = (app.consoleW - artW > 0) ? app.consoleW - artW : 0;
    const int minY = app.marqueeTop;
    const int maxY = (app.marqueeBottom - artH + 1 > minY)
        ? app.marqueeBottom - artH + 1
        : minY;

    app.x += app.vx;
    app.y += app.vy;

    if (app.x < 0 || app.x > maxX) {
        app.vx = -app.vx;
        app.x = (app.x < 0) ? 0 : maxX;
    }
    if (app.y < minY || app.y > maxY) {
        app.vy = -app.vy;
        app.y = (app.y < minY) ? minY : maxY;
    }
}

void getConsoleSize(int& width, int& height) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(out, &csbi)) {
        width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        width = 80;
        height = 25;
    }
}

void setCursorPosition(int x, int y) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = {static_cast<SHORT>(x), static_cast<SHORT>(y)};
    SetConsoleCursorPosition(out, pos);
}

void setCursorVisible(bool visible) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(out, &info);
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(out, &info);
}

bool isInteractiveConsole() {
    DWORD mode;
    return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode) &&
           GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
}

void putStr(std::vector<std::string>& rows, int row, int column,
            const std::string& text) {
    if (row < 0 || row >= static_cast<int>(rows.size())) {
        return;
    }

    for (int i = 0; i < static_cast<int>(text.size()); i++) {
        const int col = column + i;
        if (col >= 0 && col < static_cast<int>(rows[static_cast<size_t>(row)].size())) {
            rows[static_cast<size_t>(row)][static_cast<size_t>(col)] =
                text[static_cast<size_t>(i)];
        }
    }
}

std::string buildFrame(const AppState& app) {
    const int width = app.consoleW;
    const int height = app.consoleH;
    std::vector<std::string> rows(
        static_cast<size_t>(height), std::string(static_cast<size_t>(width), ' '));

    const std::string bar(static_cast<size_t>(width), '=');
    const std::string title = "CSOPESY MARQUEE CONSOLE";
    putStr(rows, 0, 0, "Welcome to CSOPESY!");
    putStr(rows, 2, 0, "Group developer:");
    putStr(rows, 3, 0, "Aquino, Bon");
    putStr(rows, 4, 0, "Dela Cruz, Karl Matthew");
    putStr(rows, 5, 0, "Espinosa, Jose Miguel");
    putStr(rows, 6, 0, "Pineda, Dencel");
    putStr(rows, 8, 0, "Version date: September 28, 2026");
    putStr(rows, 10, 0, bar);
    putStr(rows, 11, (width - static_cast<int>(title.size())) / 2, title);
    putStr(rows, 12, 0, bar);

    const int originX = static_cast<int>(app.x + 0.5);
    const int originY = static_cast<int>(app.y + 0.5);
    for (size_t i = 0; i < app.art.size(); i++) {
        const std::string& line = app.art[i];
        for (size_t j = 0; j < line.size(); j++) {
            if (line[j] == ' ') {
                continue;
            }

            const int row = originY + static_cast<int>(i);
            const int column = originX + static_cast<int>(j);
            if (row >= HEADER_ROWS && row < height && column >= 0 && column < width) {
                rows[static_cast<size_t>(row)][static_cast<size_t>(column)] = line[j];
            }
        }
    }

    putStr(rows, app.marqueeBottom + 1, 0, bar);

    const int statusRow = height - OUTPUT_ROWS - 2;
    const int promptRow = height - OUTPUT_ROWS - 1;
    const int outputRow = height - OUTPUT_ROWS;

    char status[160];
    std::snprintf(
        status,
        sizeof(status),
        "refresh: %d ms (%.1f FPS) | poll every %d ms | step every %d frames",
        app.refreshMs,
        app.measuredFps,
        POLL_INTERVAL_MS,
        ANIM_STEP);
    putStr(rows, statusRow, 0, std::string(status));

    const std::string prefix = "Enter a command (help for list): ";
    int available = width - static_cast<int>(prefix.size()) - 1;
    if (available < 1) {
        available = 1;
    }

    std::string visibleInput = app.currentInput;
    const size_t availableSize = static_cast<size_t>(available);
    if (visibleInput.size() > availableSize) {
        visibleInput = visibleInput.substr(visibleInput.size() - availableSize);
    }
    putStr(rows, promptRow, 0, prefix + visibleInput + "_");

    for (int i = 0; i < OUTPUT_ROWS; i++) {
        const std::string line = (i < static_cast<int>(app.output.size()))
            ? app.output[static_cast<size_t>(i)]
            : "";
        putStr(rows, outputRow + i, 0, line);
    }

    std::string frame;
    frame.reserve(static_cast<size_t>((width + 1) * height));
    for (size_t row = 0; row < rows.size(); row++) {
        frame += rows[row];
        if (row + 1 < rows.size()) {
            frame += '\n';
        }
    }
    return frame;
}

void drawFrame(const std::string& frame) {
    setCursorPosition(0, 0);
    std::cout << frame;
    std::cout.flush();
}

std::string toLower(std::string text) {
    for (char& character : text) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return text;
}

std::string trim(const std::string& text) {
    const size_t first = text.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    const size_t last = text.find_last_not_of(" \t");
    return text.substr(first, last - first + 1);
}

std::vector<std::string> helpText() {
    return {
        "Available commands:",
        "  help                   - displays the commands and its description",
        "  start_marquee          - starts the marquee animation",
        "  stop_marquee           - stops the marquee animation",
        "  set_text <text>        - accepts a text input and displays it as a marquee",
        "  set_speed <ms>         - sets the marquee animation refresh in milliseconds",
        "  exit                   - terminates the console"
    };
}

void submitCommand(AppState& app) {
    const std::string input = trim(app.currentInput);
    if (input.empty()) {
        return;
    }

    const size_t separator = input.find_first_of(" \t");
    const std::string command = toLower(input.substr(0, separator));
    const std::string argument = separator == std::string::npos
        ? ""
        : trim(input.substr(separator + 1));

    if (command == "exit") {
        if (!argument.empty()) {
            app.output = {"Usage: exit"};
        } else {
            app.running = false;
            app.output.clear();
        }
    } else if (command == "help") {
        app.output = argument.empty()
            ? helpText()
            : std::vector<std::string>{"Usage: help"};
    } else if (command == "start_marquee") {
        if (!argument.empty()) {
            app.output = {"Usage: start_marquee"};
        } else if (app.marqueeRunning) {
            app.output = {"Marquee is already running."};
        } else {
            app.marqueeRunning = true;
            app.output = {"Marquee started."};
        }
    } else if (command == "stop_marquee") {
        if (!argument.empty()) {
            app.output = {"Usage: stop_marquee"};
        } else if (!app.marqueeRunning) {
            app.output = {"Marquee is already stopped."};
        } else {
            app.marqueeRunning = false;
            app.output = {"Marquee stopped."};
        }
    } else if (command == "set_text") {
        if (argument.empty()) {
            app.output = {"Usage: set_text <text>"};
        } else {
            app.art = {argument};
            app.x = 1;
            app.y = app.marqueeTop + 1;
            app.vx = 1;
            app.vy = 1;
            app.output = {"Text set to: " + argument};
        }
    } else if (command == "set_speed") {
        char* parseEnd = nullptr;
        errno = 0;
        const long milliseconds = std::strtol(argument.c_str(), &parseEnd, 10);
        if (argument.empty() || errno == ERANGE ||
            parseEnd != argument.c_str() + argument.size() ||
            milliseconds < 1 || milliseconds > 60000) {
            app.output = {"Usage: set_speed <milliseconds> (1-60000)"};
        } else {
            app.refreshMs = static_cast<int>(milliseconds);
            app.output = {
                "Refresh set to " + std::to_string(milliseconds) + " ms."
            };
        }
    } else {
        app.output = {"Error: Unrecognized command. Type 'help' for commands."};
    }
}

void pollInput(AppState& app) {
    while (_kbhit()) {
        const int character = _getch();
        if (character == 0 || character == 224) {
            if (_kbhit()) {
                (void)_getch();
            }
            continue;
        }

        if (character == '\r' || character == '\n') {
            submitCommand(app);
            app.currentInput.clear();
        } else if (character == '\b' || character == 127) {
            if (!app.currentInput.empty()) {
                app.currentInput.pop_back();
            }
        } else if (character >= 32 && character < 127) {
            app.currentInput.push_back(static_cast<char>(character));
        }
    }
}

int main() {
    AppState app;
    int width, height;
    getConsoleSize(width, height);

    app.consoleW = (DESIRED_W < width - 1) ? DESIRED_W : width - 1;
    if (app.consoleW < 20) {
        app.consoleW = 20;
    }
    app.consoleH = (DESIRED_H < height) ? DESIRED_H : height;
    const int minimumHeight =
        HEADER_ROWS + static_cast<int>(BIG_CSOPESY.size()) + OUTPUT_ROWS + 3;
    if (app.consoleH < minimumHeight) {
        app.consoleH = minimumHeight;
    }

    app.marqueeTop = HEADER_ROWS;
    app.marqueeBottom = app.consoleH - OUTPUT_ROWS - 4;
    app.x = 1;
    app.y = HEADER_ROWS + 1;
    app.vx = 1;
    app.vy = 1;
    app.running = true;
    app.marqueeRunning = false;
    app.measuredFps = 0.0;
    app.refreshMs = DEFAULT_REFRESH_MS;
    app.art = BIG_CSOPESY;
    app.output.push_back("Type 'help' for commands or 'exit' to quit.");

    if (!isInteractiveConsole()) {
        std::string line;
        while (app.running && std::getline(std::cin, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            app.currentInput = line;
            submitCommand(app);
            for (const std::string& message : app.output) {
                std::cout << message << '\n';
            }
            app.currentInput.clear();
        }
        return 0;
    }

    std::system("cls");
    setCursorVisible(false);

    using Clock = std::chrono::steady_clock;
    Clock::time_point lastPoll = Clock::now();
    Clock::time_point lastDraw = Clock::now();
    Clock::time_point fpsMark = Clock::now();
    int frames = 0;
    int animationFrame = 0;

    while (app.running) {
        const Clock::time_point now = Clock::now();

        if (std::chrono::duration<double, std::milli>(now - lastPoll).count() >=
            POLL_INTERVAL_MS) {
            pollInput(app);
            lastPoll = now;
        }

        if (std::chrono::duration<double, std::milli>(now - lastDraw).count() >=
            app.refreshMs) {
            if (app.marqueeRunning && animationFrame % ANIM_STEP == 0) {
                updateMotion(app);
            }
            animationFrame++;

            frames++;
            const double elapsed =
                std::chrono::duration<double>(now - fpsMark).count();
            if (elapsed >= 0.5) {
                app.measuredFps = frames / elapsed;
                frames = 0;
                fpsMark = now;
            }

            drawFrame(buildFrame(app));
            lastDraw = now;
        }

        Sleep(BASE_SLEEP_MS > 0 ? static_cast<DWORD>(BASE_SLEEP_MS) : 0);
    }

    setCursorVisible(true);
    setCursorPosition(0, app.consoleH);
    std::cout << std::endl;
    return 0;
}
