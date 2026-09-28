#include "Marquee.h"

// FIGlet BIG rendering of CSOPESY. Motion follows Bon Aquino's submission.
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
    for (std::size_t i = 0; i < app.art.size(); i++) {
        if (static_cast<int>(app.art[i].size()) > width) {
            width = static_cast<int>(app.art[i].size());
        }
    }
    return width;
}

void updateMotion(AppState& app, double dt) {
    int artW = logoWidth(app);
    int artH = static_cast<int>(app.art.size());
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
