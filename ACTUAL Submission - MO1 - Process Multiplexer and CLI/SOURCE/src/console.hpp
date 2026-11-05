#pragma once

#include <chrono>

namespace csopesy {

struct ClockPolicy {
    std::chrono::milliseconds generating{5};
    std::chrono::milliseconds stopped{5};
    const char* explanation = nullptr;
};

int runConsole(int argc, char** argv, ClockPolicy policy = {});

} // namespace csopesy
