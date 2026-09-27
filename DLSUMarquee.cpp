// CSOPESY S09 Week 5 Marquee Console - Bon Aquino
// A real-time marquee console: where the DLSU seal glides across the screen while the
// keyboard is polled live

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdio> 
#include <cctype>    // for tolower
#include <conio.h>    //  for keyboard polling
#include <windows.h>  // for console cursor, size, Sleep
#include <mmsystem.h> // for sleep 1ms

using namespace std;

const double TARGET_FPS       = 60.0; // refresh rate. >60 is wasted on a 60Hz panel
const int    POLL_INTERVAL_MS = 1;    // how often we check keyboard
const int    BASE_SLEEP_MS    = 1;    // base loop tick
const double H_SPEED          = 26.0; // marquee horizontal speed (cells per second)
const double V_SPEED          = 11.0; // marquee vertical speed (cells per second)

const int DESIRED_W   = 90;
const int DESIRED_H   = 30;
const int HEADER_ROWS = 3; // top banner
const int OUTPUT_ROWS = 4; // bottom area for help text / echo

//ascii art
const vector<string> DLSU_LOGO = {
    "       .---------------------.",
    "      /                       \\",
    "     /       DE LA SALLE       \\",
    "    /    U N I V E R S I T Y    \\",
    "   |              *              |",
    "   |             ***             |",
    "   |            *****            |",
    "   |        *** ***** ***        |",
    "   |         ***********         |",
    "   |          *********          |",
    "   |          ***   ***          |",
    "   |         ***     ***         |",
    "   |        **         **        |",
    "   |    RELIGIO MORES CULTURA    |",
    "   |           MANILA            |",
    "    \\                           /",
    "     \\                         /",
    "      \\                       /",
    "       '---------------------'"
};

struct AppState {
    int consoleW, consoleH;        // usable console size
    int marqueeTop, marqueeBottom; // rows the seal may travel between
    double x, y;                   // seal top-left position (kept as double for smooth motion)
    double vx, vy;                 // direction, either +1 or -1
    string currentInput;           // what the user is typing right now
    vector<string> output;         // help text or the last echoed line
    bool running;
    double measuredFps;            // live, for the status line
};

// prototypes (so the functions can call each other in a readable order)
void submitCommand(AppState& app);
int  logoWidth();
void putStr(vector<string>& rows, int r, int c, const string& s);

void getConsoleSize(int& w, int& h) { // ask the console how big it is right now
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleScreenBufferInfo(out, &csbi)) {
        w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        w = 80; h = 25; // safe fallback
    }
}

void setCursorPosition(int x, int y) { // move the cursor without clearing the screen
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    COORD pos = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(out, pos);
}

void setCursorVisible(bool visible) { // hide the cursor so it doesn't blink mid-frame
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    GetConsoleCursorInfo(out, &info);
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(out, &info);
}

string toLower(string s) { // for case-insensitive command matching
    for (char& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

string trim(const string& s) { // strip surrounding spaces/tabs
    size_t a = s.find_first_not_of(" \t");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t");
    return s.substr(a, b - a + 1);
}

int logoWidth() { // widest line of the seal
    int w = 0;
    for (size_t i = 0; i < DLSU_LOGO.size(); i++)
        if ((int)DLSU_LOGO[i].size() > w) w = (int)DLSU_LOGO[i].size();
    return w;
}

void putStr(vector<string>& rows, int r, int c, const string& s) { // blit text into the grid, clipped
    if (r < 0 || r >= (int)rows.size()) return;
    for (int i = 0; i < (int)s.size(); i++) {
        int col = c + i;
        if (col < 0 || col >= (int)rows[(size_t)r].size()) continue;
        rows[(size_t)r][(size_t)col] = s[(size_t)i];
    }
}

vector<string> helpText() { // 'help' shows this info about the program
    vector<string> h;
    h.push_back("DLSU MARQUEE CONSOLE:");
    h.push_back("  DLSU logo animated in real time + supports keyboard polling.");
    h.push_back("  Commands:  help = show about    exit = quit program.");
    h.push_back("  Anything else typed here is simply echoed back.");
    return h;
}

void submitCommand(AppState& app) { // runs when the user presses Enter
    string cmd = toLower(trim(app.currentInput));
    if (cmd.empty()) return;            // ignore empty lines (e.g. a stray Enter from a paste)
    if (cmd == "exit") {
        app.running = false;
    } else if (cmd == "help") {
        app.output = helpText();
    } else {
        app.output.clear();             // everything else just echoes what was typed
        app.output.push_back("you typed: " + trim(app.currentInput));
    }
}

void pollInput(AppState& app) { // drain the whole keyboard buffer this tick
    while (_kbhit()) {
        int ch = _getch();
        if (ch == 0 || ch == 224) {              // arrow / function key: consume the 2nd byte + ignore
            if (_kbhit()) _getch();
            continue;
        }
        if (ch == '\r' || ch == '\n') {          // Enter -> submit (newlines are never stored)
            submitCommand(app);
            app.currentInput.clear();
        } else if (ch == '\b' || ch == 127) {    // Backspace
            if (!app.currentInput.empty()) app.currentInput.pop_back();
        } else if (ch >= 32 && ch < 127) {       // printable ASCII only
            app.currentInput.push_back((char)ch);
        }
        // any other control char is dropped -> pasting multi-line text can't break the layout
    }
}

void updateMotion(AppState& app, double dt) { // move the seal and bounce off the walls
    int artW = logoWidth();
    int artH = (int)DLSU_LOGO.size();
    int maxX = app.consoleW - artW;          if (maxX < 0) maxX = 0;
    int minY = app.marqueeTop;
    int maxY = app.marqueeBottom - artH + 1; if (maxY < minY) maxY = minY;

    app.x += app.vx * H_SPEED * dt;
    app.y += app.vy * V_SPEED * dt;

    if (app.x <= 0)    { app.x = 0;    app.vx =  1; }
    if (app.x >= maxX) { app.x = maxX; app.vx = -1; }
    if (app.y <= minY) { app.y = minY; app.vy =  1; }
    if (app.y >= maxY) { app.y = maxY; app.vy = -1; }
}

string buildFrame(const AppState& app) { // compose the WHOLE screen into one string (back buffer)
    int W = app.consoleW;
    int H = app.consoleH;
    vector<string> rows((size_t)H, string((size_t)W, ' '));

    // header banner
    string bar((size_t)W, '=');
    string title = "DLSU MARQUEE CONSOLE   -   CSOPESY S09   (Bon Aquino)";
    putStr(rows, 0, 0, bar);
    putStr(rows, 1, (W - (int)title.size()) / 2, title);
    putStr(rows, 2, 0, bar);

    // the marquee seal (only non-space cells, so it overlaps cleanly)
    int ox = (int)(app.x + 0.5);
    int oy = (int)(app.y + 0.5);
    for (size_t i = 0; i < DLSU_LOGO.size(); i++) {
        const string& line = DLSU_LOGO[i];
        for (size_t j = 0; j < line.size(); j++) {
            if (line[j] == ' ') continue;
            int r = oy + (int)i;
            int c = ox + (int)j;
            if (r >= HEADER_ROWS && r < H && c >= 0 && c < W)
                rows[(size_t)r][(size_t)c] = line[j];
        }
    }

    // status line (live refresh + poll values)
    int statusRow = H - 6;
    int promptRow = H - 5;
    int outRow0   = H - OUTPUT_ROWS;
    char buf[160];
    snprintf(buf, sizeof(buf),
        "refresh: %.1f FPS (target %d)  |  poll every %d ms  |  speed %.0f cps",
        app.measuredFps, (int)TARGET_FPS, POLL_INTERVAL_MS, H_SPEED);
    putStr(rows, statusRow, 0, string(buf));

    // command prompt + current input (single line, tail-scrolled if too long)
    string prefix = "Enter a command (help / exit): ";
    int avail = W - (int)prefix.size() - 1;
    if (avail < 1) avail = 1;
    string vis = app.currentInput;
    string::size_type availU = (string::size_type)avail; // avail is >= 1 here
    if (vis.size() > availU) vis = vis.substr(vis.size() - availU);
    putStr(rows, promptRow, 0, prefix + vis + "_");

    // output area: help text or the echoed line
    for (int k = 0; k < OUTPUT_ROWS; k++) {
        string line = (k < (int)app.output.size()) ? app.output[(size_t)k] : "";
        putStr(rows, outRow0 + k, 0, line);
    }

    // join rows into one buffer (no trailing newline -> the console never scrolls)
    string frame;
    frame.reserve((size_t)((W + 1) * H));
    for (size_t r = 0; r < rows.size(); r++) {
        frame += rows[r];
        if (r + 1 < rows.size()) frame += '\n';
    }
    return frame;
}

void drawFrame(const string& frame) { // home the cursor and write the whole frame at once
    setCursorPosition(0, 0);
    cout << frame;
    cout.flush();
}

int main() {
    AppState app;
    int w, h;
    getConsoleSize(w, h);
    app.consoleW = (DESIRED_W < w - 1) ? DESIRED_W : (w - 1); if (app.consoleW < 20) app.consoleW = 20;
    app.consoleH = (DESIRED_H < h)     ? DESIRED_H : h;       if (app.consoleH < 18) app.consoleH = 18;
    app.marqueeTop    = HEADER_ROWS;
    app.marqueeBottom = app.consoleH - 7;
    app.x = 1;
    app.y = HEADER_ROWS + 1;
    app.vx = 1;
    app.vy = 1;
    app.running = true;
    app.measuredFps = 0.0;
    app.output.push_back("Type 'help' for info or 'exit' to quit or type anything while the animation moves");

    timeBeginPeriod(1);       // 1ms timer resolution -> Sleep(1) ~ 1ms, so refresh/poll rates are honest
    system("cls");            // clear ONCE only (per-frame cls is what causes flicker)
    setCursorVisible(false);

    typedef chrono::steady_clock Clock;
    Clock::time_point lastPoll = Clock::now();
    Clock::time_point lastDraw = Clock::now();
    Clock::time_point lastMove = Clock::now();
    Clock::time_point fpsMark  = Clock::now();
    int frames = 0;
    const double frameInterval = 1.0 / TARGET_FPS;

    while (app.running) {
        Clock::time_point now = Clock::now();

        // ---- POLL on its own cadence (raise POLL_INTERVAL_MS to feel typing lag) ----
        if (chrono::duration<double, milli>(now - lastPoll).count() >= POLL_INTERVAL_MS) {
            pollInput(app);
            lastPoll = now;
        }

        // ---- REFRESH on its own cadence (raise TARGET_FPS / set BASE_SLEEP_MS=0 to see tearing) ----
        if (chrono::duration<double>(now - lastDraw).count() >= frameInterval) {
            double dt = chrono::duration<double>(now - lastMove).count();
            lastMove = now;
            updateMotion(app, dt);

            frames++;
            double since = chrono::duration<double>(now - fpsMark).count();
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

    // tidy up so the terminal is usable again
    setCursorVisible(true);
    setCursorPosition(0, app.consoleH);
    cout << endl;
    timeEndPeriod(1);         // release the high-resolution timer
    return 0;
}
