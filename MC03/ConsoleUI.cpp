#include "ConsoleUI.h"

#include <windows.h>

#include <cstdio>
#include <iostream>

void getConsoleSize(int& w, int& h) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(out, &csbi)) {
        w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        w = 80; h = 25;
    }
}

void setCursorPosition(int x, int y) {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)x, (SHORT)y };
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

void putStr(std::vector<std::string>& rows, int r, int c, const std::string& s) {
    if (r < 0 || r >= (int)rows.size()) return;
    for (int i = 0; i < (int)s.size(); i++) {
        int col = c + i;
        if (col < 0 || col >= (int)rows[(size_t)r].size()) continue;
        rows[(size_t)r][(size_t)col] = s[(size_t)i];
    }
}

std::string buildFrame(const AppState& app) {
    int W = app.consoleW;
    int H = app.consoleH;
    std::vector<std::string> rows((size_t)H, std::string((size_t)W, ' '));

    std::string bar((size_t)W, '=');
    std::string title = "CSOPESY MARQUEE CONSOLE";
    putStr(rows, 0, 0, bar);
    putStr(rows, 1, (W - (int)title.size()) / 2, title);
    putStr(rows, 2, 0, bar);

    int ox = (int)(app.x + 0.5);
    int oy = (int)(app.y + 0.5);
    for (size_t i = 0; i < app.art.size(); i++) {
        const std::string& line = app.art[i];
        for (size_t j = 0; j < line.size(); j++) {
            if (line[j] == ' ') continue;
            int r = oy + (int)i;
            int c = ox + (int)j;
            if (r >= HEADER_ROWS && r < H && c >= 0 && c < W)
                rows[(size_t)r][(size_t)c] = line[j];
        }
    }

    int statusRow = H - OUTPUT_ROWS - 2;
    int promptRow = H - OUTPUT_ROWS - 1;
    int outRow0   = H - OUTPUT_ROWS;
    char buf[160];
    std::snprintf(buf, sizeof(buf),
        "refresh: %d ms (%.1f FPS) | poll every %d ms | speed %.0f cps",
        app.refreshMs, app.measuredFps, POLL_INTERVAL_MS, H_SPEED);
    putStr(rows, statusRow, 0, std::string(buf));

    std::string prefix = "Enter a command (help for list): ";
    int avail = W - (int)prefix.size() - 1;
    if (avail < 1) avail = 1;
    std::string vis = app.currentInput;
    std::string::size_type availU = (std::string::size_type)avail;
    if (vis.size() > availU) vis = vis.substr(vis.size() - availU);
    putStr(rows, promptRow, 0, prefix + vis + "_");

    for (int k = 0; k < OUTPUT_ROWS; k++) {
        std::string line = (k < (int)app.output.size()) ? app.output[(size_t)k] : "";
        putStr(rows, outRow0 + k, 0, line);
    }

    std::string frame;
    frame.reserve((size_t)((W + 1) * H));
    for (size_t r = 0; r < rows.size(); r++) {
        frame += rows[r];
        if (r + 1 < rows.size()) frame += '\n';
    }
    return frame;
}

void drawFrame(const std::string& frame) {
    setCursorPosition(0, 0);
    std::cout << frame;
    std::cout.flush();
}
