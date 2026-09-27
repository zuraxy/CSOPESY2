# ImplementationPlan.md — DLSU Marquee Console

**Project:** CSOPESY S09 Week 5 — Marquee Console
**Author:** Bon Aquino
**Date:** 2026-06-13
**Goal:** A simple, single-file C++ console app that animates ASCII art of the DLSU
logo (marquee) with an independently tunable refresh rate and keyboard polling rate,
documented tearing/lag limits, `help` + `exit` commands, and crash-proof input.

---

## 1. Hard constraints discovered (these shape every decision)

| Constraint | Evidence | Consequence |
|---|---|---|
| Build compiler is **TDM-GCC 4.9.2** | `g++ --version` → `g++.exe (tdm64-1) 4.9.2`; `settings.json` `cppCompilerPath: "g++"` | **Target C++11 only.** No C++14/17/20 (`std::optional`, structured bindings, `string_view`, `unordered_map::contains`). |
| VS Code build = **C/C++ Runner extension**, single-file run | `.vscode/settings.json` (`C_Cpp_Runner.*`), `useMsvc:false` | The ▶ "Run C/C++ File" button compiles **only the active file** → use **one self-contained `.cpp`** so the other `main()`s (W4, Threads, etc.) don't collide. |
| `cppStandard` is empty | `settings.json` `"C_Cpp_Runner.cppStandard": ""` | Recommend setting it to `"c++11"` for a deterministic build; code must still build at `-std=c++11`. |
| Output path | `launch.json` `program: build/Debug/outDebug` | C/C++ Runner emits here; nothing extra to configure. |
| Reference impl is **MSVC**, not portable | `MarqueeConsoleWrongImplementation/*.sln`, `vc143`, uses `.contains()` | Do **not** reuse it; rebuild simply, original code. |
| **Avoid** `WrongImplementation-(24of30).cpp` | User instruction; also `#include "display/DisplayHandler.h"` which does not exist | Don't copy its DisplayHandler/modulo-counter structure. |

## 2. What the 13 lecture images require (triage)

**Use (taught for this exact task):**
- **Game loop** (img 4): `targetFPS`, `frameTime = 1/FPS`, loop `{ input; update; render; Sleep }` → refresh model.
- **Keyboard polling** (img 2/6/8): `_kbhit()` non-blocking + `_getch()` blocking → the graded "I/O polling mechanism".
- **`setCursorPosition`** (img 11): `COORD` + `SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), …)` → flicker-free redraw, **no `system("cls")`**.
- **Marquee execution sequence** (img 10): header → move (x,y) → bounds check → command field → poll → check hit. Single-threaded blueprint.

**Skip (future scope / examples only):** IMGUI/Dear ImGui (img 1/12/13), Qt (img 4/5),
ConsoleManager/AConsole/FileSystem/MemoryManager OOP (img 3/13),
GetAsyncKeyState key up/down events (img 8/9), five-phase OS loading (img 13).

## 3. Architecture — one file, single-threaded, time-decoupled

`DLSUMarquee.cpp` (project root). Sections:

1. **Includes / config constants** — `<iostream> <string> <vector> <chrono> <conio.h> <windows.h>`; `using namespace std;`
2. **DLSU art** — `vector<string> DLSU_LOGO` embedded in-source (no external file → no CWD fragility).
3. **Windows helpers** — `setCursorPosition(x,y)`, `setCursorVisible(bool)`, `consoleTextColor(int)` (mirrors `W4/main0.cpp`).
4. **Render** — `buildFrame()` composes the **entire screen into one string** (header + logo blitted at (x,y) + command line + live status), padding every line to full width; `drawFrame()` homes the cursor to (0,0) and writes it **once**.
5. **Animation** — position from **elapsed time** (cells/sec) so motion speed is constant regardless of FPS; bounce off bounds.
6. **Polling** — `pollInput()` drains the buffer with `while(_kbhit()) _getch()`; printable→append, Backspace→pop, Enter→submit; **newlines/control chars stripped**.
7. **Commands** — `help` (program info), `exit` (quit); **anything else is echoed back** (no crash).
8. **`main()`** — game loop: poll every ~1 ms tick; redraw only when the frame interval has elapsed; `Sleep(1)` base tick; restore cursor on exit.

### Why single-threaded + decoupled timing
- Matches img 10 (sequential loop) and keeps it simple.
- The reference's bug was **coupling** poll to a 500 ms refresh → laggy typing. We separate them with two `steady_clock` timestamps: poll runs ~1000 Hz, redraw runs at `TARGET_FPS`.
- One writer → **no two-thread tearing**; back-buffer + cursor-home → **no `cls` flicker**.

## 4. Refresh / poll balance (60 Hz panel, VS Code integrated terminal)

| Knob | Constant | Recommended | Rationale |
|---|---|---|---|
| Refresh | `TARGET_FPS` | **60** | Matches the 60 Hz panel; refreshing faster is wasted on a 60 Hz display. |
| Poll | `POLL_INTERVAL_MS` / base `Sleep` | **1 ms** (~1000 Hz) | `_kbhit` drain each tick → typing feels instant. |
| Motion speed | `MARQUEE_CELLS_PER_SEC` | ~20–30 | Readable glide, independent of FPS. |

On-screen **live FPS + poll interval** readout so the limits can be observed directly.

### Limits to identify & record (→ ProjectState.md after testing)
- **Refresh / tearing:** raise `TARGET_FPS` (120 / 240 / uncapped `Sleep(0)`). On a 60 Hz panel >60 FPS yields no visual gain; uncapped redraw spikes CPU and can show partial-frame tearing. Record the FPS where tearing/flicker first appears in the VS Code terminal.
- **Poll / typing lag:** raise `POLL_INTERVAL_MS` (50 / 100 / 200 ms). Record where keystrokes feel delayed / get dropped during fast typing.

## 5. Multi-line handling (chosen: strip newlines, single line)
- Input buffer accepts only printable ASCII (32–126); `\r`/`\n`/control chars are **never stored** → submit on Enter.
- Command line renders on **one line**; if input exceeds the visible width, show the trailing portion (horizontal scroll) so layout never breaks.
- Pasted multi-line text → each embedded Enter submits a line; nothing crashes. Only `help`/`exit` are recognized.

## 6. Coding-style conformance (from `W4/` + `Threads/`)
- Top header comment: `// CSOPESY S09 Week 5 Marquee Console - Bon Aquino`.
- `using namespace std;`, 4-space indent, K&R braces, casual lowercase trailing `//` comments.
- Windows API cursor helpers with `(SHORT)` casts (as in `W4/main0.cpp`).
- Small single-purpose functions; C++11 only.

## 7. DLSU ASCII art
- Hand-rendered seal derived from `DLSU-Logo.png`: central La Salle **star**, ring text
  **DE LA SALLE UNIVERSITY** / **MANILA**, inner **RELIGIO MORES CULTURA**, seal border.
- Sized to glide inside a ~120×30 console (≈ ≤46 wide × ≤16 tall). Visually verified before embedding.

## 8. Run instructions
1. Open `DLSUMarquee.cpp` in VS Code.
2. Click the C/C++ Runner ▶ **"Run C/C++ File"** (compiles active file → `build/Debug/outDebug` → runs in integrated terminal).
3. (Recommended) set `C_Cpp_Runner.cppStandard` to `"c++11"` for a deterministic build.

## 9. Validation
- Self-compile with the project's exact flags:
  `g++ -std=c++11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion DLSUMarquee.cpp -o build/Debug/outDebug.exe` → must be clean.
- Manual checks: logo glides & bounces; typing is responsive; `help` lists commands; `exit` restores cursor and quits; pasting multi-line text does not crash; long input scrolls on one line.
- Sweep `TARGET_FPS` / `POLL_INTERVAL_MS` to record limits.

## 10. Risks
| Risk | Likelihood | Mitigation |
|---|---|---|
| GCC 4.9.2 rejects modern C++ | High | C++11 only; self-compile before delivery |
| Empty `cppStandard` → gnu++98 default breaks `<chrono>` | Medium | Recommend `cppStandard: "c++11"`; verify |
| Folder-mode build hits multiple `main()`s | Medium | Single-file run via ▶ button; documented |
| Tearing differs across terminals | Low (target chosen) | Back-buffer + cursor-home; document VS Code-terminal thresholds |

## 11. Task checklist
- [ ] Embed verified DLSU ASCII art
- [ ] Windows helpers (cursor pos / visibility / color)
- [ ] `buildFrame()` + `drawFrame()` (back-buffer, full-width padding)
- [ ] Time-based animation + bounce
- [ ] `pollInput()` drain + newline stripping
- [ ] `help` / `exit` + graceful unknown
- [ ] Game loop with decoupled refresh/poll + live FPS readout
- [ ] Compile clean at `-std=c++11` with project warnings
- [ ] Record measured limits → ProjectState.md
