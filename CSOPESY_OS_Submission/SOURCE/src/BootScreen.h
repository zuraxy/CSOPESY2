#pragma once

#include "OsState.h"

namespace BootScreen {
// Renders the fake boot sequence (BIOS POST, then OS splash) and advances
// state.bootPhase / state.bootElapsed. Call instead of the desktop until
// state.bootPhase == OsState::BootPhase::Done.
void Render(OsState& state);
}
