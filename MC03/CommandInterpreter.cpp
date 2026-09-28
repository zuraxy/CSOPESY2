#include "CommandInterpreter.h"

#include <charconv>
#include <cctype>
#include <conio.h>
#include <system_error>

std::string toLower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> helpText() {
    std::vector<std::string> h;
    h.push_back("Available commands:");
    h.push_back("  help                   - display commands and descriptions");
    h.push_back("  start_marquee          - start the marquee animation");
    h.push_back("  stop_marquee           - pause the marquee animation");
    h.push_back("  set_text <text>        - display new text as the marquee");
    h.push_back("  set_speed <ms>         - set redraw interval in milliseconds");
    h.push_back("  exit                   - terminate the console");
    return h;
}

void submitCommand(AppState& app) {
    const std::string input = trim(app.currentInput);
    if (input.empty()) return;

    const size_t separator = input.find_first_of(" \t");
    const std::string command = toLower(input.substr(0, separator));
    const std::string argument = separator == std::string::npos
        ? "" : trim(input.substr(separator + 1));

    if (command == "exit") {
        if (!argument.empty()) app.output = {"Usage: exit"};
        else {
            app.running = false;
            app.output.clear();
        }
    } else if (command == "help") {
        app.output = argument.empty() ? helpText()
                                      : std::vector<std::string>{"Usage: help"};
    } else if (command == "start_marquee") {
        if (!argument.empty()) app.output = {"Usage: start_marquee"};
        else if (app.marqueeRunning) app.output = {"Marquee is already running."};
        else {
            app.marqueeRunning = true;
            app.output = {"Marquee started."};
        }
    } else if (command == "stop_marquee") {
        if (!argument.empty()) app.output = {"Usage: stop_marquee"};
        else if (!app.marqueeRunning) app.output = {"Marquee is already stopped."};
        else {
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
        int milliseconds = 0;
        const char* begin = argument.data();
        const char* end = begin + argument.size();
        const auto parsed = std::from_chars(begin, end, milliseconds);
        if (argument.empty() || parsed.ec != std::errc() ||
            parsed.ptr != end || milliseconds < 1 || milliseconds > 60000) {
            app.output = {"Usage: set_speed <milliseconds> (1-60000)"};
        } else {
            app.refreshMs = milliseconds;
            app.output = {"Refresh set to " + std::to_string(milliseconds) + " ms."};
        }
    } else {
        app.output = {"Error: Unrecognized command. Type 'help' for commands."};
    }
}

void pollInput(AppState& app) {
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {
            if (_kbhit()) _getch();
            continue;
        }
        if (ch == '\r' || ch == '\n') {
            submitCommand(app);
            app.currentInput.clear();
        } else if (ch == '\b' || ch == 127) {
            if (!app.currentInput.empty()) app.currentInput.pop_back();
        } else if (ch >= 32 && ch < 127) {
            app.currentInput.push_back((char)ch);
        }
    }
}
