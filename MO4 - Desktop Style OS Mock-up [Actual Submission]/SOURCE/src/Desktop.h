#pragma once

#include "OsState.h"

namespace Desktop {
// Renders the fullscreen wallpaper layer, real-time clock, and PWR button.
// Must be called first each frame so it sits behind every other window.
void Render(OsState& state);
}
