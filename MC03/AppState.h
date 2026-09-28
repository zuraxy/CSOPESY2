#ifndef APP_STATE_H
#define APP_STATE_H

#include <string>
#include <vector>

const double TARGET_FPS       = 60.0;
const int    POLL_INTERVAL_MS = 1;
const int    BASE_SLEEP_MS    = 1;
const double H_SPEED          = 26.0;
const double V_SPEED          = 11.0;

const int DESIRED_W   = 90;
const int DESIRED_H   = 30;
const int HEADER_ROWS = 3;
const int OUTPUT_ROWS = 7;

struct AppState {
    int consoleW, consoleH;
    int marqueeTop, marqueeBottom;
    double x, y;
    double vx, vy;
    std::string currentInput;
    std::vector<std::string> output;
    std::vector<std::string> art;
    bool running;
    bool marqueeRunning;
    double measuredFps;
    int refreshMs;
};

#endif
