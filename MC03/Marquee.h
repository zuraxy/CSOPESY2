#ifndef MARQUEE_H
#define MARQUEE_H

#include "AppState.h"

extern const std::vector<std::string> BIG_CSOPESY;

int logoWidth(const AppState& app);
void updateMotion(AppState& app, double dt);

#endif
