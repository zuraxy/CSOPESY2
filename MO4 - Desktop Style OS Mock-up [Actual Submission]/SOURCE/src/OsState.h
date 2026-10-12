#pragma once

// Shared state of the OS shell, passed to every component each frame.
struct OsState {
    // Boot sequence: BIOS POST -> OS splash -> Done (desktop visible).
    enum class BootPhase { Post, Splash, Done };
    BootPhase bootPhase   = BootPhase::Post;
    float     bootElapsed = 0.0f;   // seconds elapsed in the current phase

    bool showFileExplorer  = false;
    bool showSettings      = false;
    bool showTaskManager   = false;
    bool shutdownRequested = false;
};
