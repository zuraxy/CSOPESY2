# ProjectState.md — DLSU Marquee Console

**Status:** Implemented and building clean on the real toolchain (TDM-GCC 4.9.2, C++11).
**Author:** Bon Aquino — CSOPESY S09 Week 5
**Date:** 2026-06-13
**Entry point:** [DLSUMarquee.cpp](DLSUMarquee.cpp)

---

## 1. What was implemented

A single-file, real-time marquee console:

- **Marquee animation** — a symmetric ASCII DLSU seal (with the La Salle star, `DE LA SALLE
  UNIVERSITY`, `RELIGIO MORES CULTURA`, `MANILA`) glides and bounces off all four walls.
  Motion is **time-based** (cells/second), so speed is constant no matter the frame rate.
- **I/O polling** — `_kbhit()` / `_getch()` drain the keyboard every tick; you can type while
  the seal keeps moving (real-time, immediate-mode — exactly the lecture's model).
- **Decoupled refresh vs poll** — two independent `steady_clock` timers, so the refresh rate
  and polling rate can be tuned and stressed **separately**.
- **Flicker/tear-free rendering** — every frame is composed into one back-buffer string and
  written once after homing the cursor to (0,0); **no per-frame `system("cls")`**; the cursor
  is hidden during animation. `timeBeginPeriod(1)` gives honest ~1 ms timing.
- **Commands:** `help` (prints program info), `exit` (quits), **everything else is echoed back**.
- **Multi-line safe:** only printable ASCII is stored; newlines/control chars are stripped, so
  pasting multi-line text or typing past the edge can never break the layout or crash.

## 2. Files

| File | State | Notes |
|---|---|---|
| `DLSUMarquee.cpp` | **created** | The whole program (~290 lines, C++11, one `main`). |
| `.vscode/settings.json` | **edited (3 keys)** | Required to build at all — see §4. |
| `ImplementationPlan.md` | created (planning) | The pre-implementation plan. |
| `ProjectState.md` | this file | Post-implementation record. |
| `_art_check.txt` | created then deleted | Throwaway used only to verify art symmetry. |

## 3. How to run (VS Code)

1. Open `DLSUMarquee.cpp`.
2. Click the **C/C++ Runner ▶ "Run C/C++ File"** button (top-right) — it compiles the active
   file to `build/Debug/outDebug` and runs it in the integrated terminal.
3. Maximize / enlarge the terminal panel a bit for the full 90×30 layout (it auto-clamps to the
   actual console size, so a smaller panel still works, just with less room to move).
4. Type `help`, type anything to see it echoed, type `exit` to quit.

Equivalent CLI build (what the Runner now does under the hood):
```
g++ -std=c++11 -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wcast-align -Wconversion -Wsign-conversion DLSUMarquee.cpp -o build/Debug/outDebug.exe -lwinmm
```

## 4. settings.json changes (and why they were mandatory)

Your build is the **C/C++ Runner** extension with `g++` = **TDM-GCC 4.9.2**. As shipped, the config
could not compile *any* C++11 file. Three minimal, reversible edits:

| Key | Before | After | Why |
|---|---|---|---|
| `C_Cpp_Runner.cppStandard` | `""` | `"c++11"` | Empty → GCC defaulted to C++98 → `<chrono>` hard-errored (`#error … requires -std=c++11`). |
| `C_Cpp_Runner.warnings` | included `-Wnull-dereference` | removed it | GCC 4.9.2 doesn't know that flag (added in GCC 6) → `error: unrecognized command line option` → build aborted. |
| `C_Cpp_Runner.linkerArgs` | `[]` | `["-lwinmm"]` | Links `timeBeginPeriod` (1 ms timer). Harmless for your other files. |

**To revert:** restore those three values. (If you remove `-lwinmm`, also delete the
`timeBeginPeriod/timeEndPeriod` lines and `#include <mmsystem.h>` from `DLSUMarquee.cpp`.)

## 5. Refresh rate vs polling rate

The knobs live at the top of `DLSUMarquee.cpp`:

```cpp
const double TARGET_FPS       = 60.0; // refresh rate
const int    POLL_INTERVAL_MS = 1;    // keyboard poll period (~1000 Hz)
const int    BASE_SLEEP_MS    = 1;    // base loop tick (0 = busy-wait, for high-FPS testing)
```

**Recommended for your hardware (60 Hz panel, VS Code integrated terminal):**
- `TARGET_FPS = 60` — matches the 60 Hz panel; anything above is invisible to a 60 Hz display.
- `POLL_INTERVAL_MS = 1` — typing is indistinguishable from instant.
- `BASE_SLEEP_MS = 1` — keeps CPU usage low while holding 60 FPS (thanks to the 1 ms timer).

A live readout (`refresh: NN.N FPS | poll every N ms | speed NN cps`) is shown above the prompt so
you can watch the effect of any change in real time.

## 6. Limits to confirm on your machine (method + expected results)

These need your eyes on the actual terminal (a screen-tear/lag is a visual/feel judgment). Rebuild
after each change and observe:

| Experiment | Change | Expected limit |
|---|---|---|
| **Refresh / tearing** | `BASE_SLEEP_MS = 0`, then `TARGET_FPS = 120`, `240`, `1000` | On a 60 Hz panel, >60 FPS shows no improvement; pushed high, full-frame writes outrun the terminal → **partial-frame tearing/flicker appears (roughly ~100–150+ FPS in the VS Code terminal)** and one CPU core hits ~100% (busy-wait — the lecture's cited downside of polling). |
| **Polling / typing lag** | `POLL_INTERVAL_MS = 50`, `100`, `200` | Typing stays fine to ~30–50 ms; **noticeable delay / dropped fast keystrokes appears around ~100 ms+** (≤10 polls/sec). |

Fill in your observed thresholds here after testing:

```
My monitor: 60 Hz | Terminal: VS Code integrated
Tearing first seen at TARGET_FPS = ____  (BASE_SLEEP_MS = 0)
Typing lag first felt at POLL_INTERVAL_MS = ____ ms
Chosen balance: TARGET_FPS = 60, POLL_INTERVAL_MS = 1
```

## 7. Verification done

- ✅ **Compiles clean** (exit 0, zero warnings) on TDM-GCC 4.9.2 with the project's full warning set.
- ✅ **ASCII art symmetry proven** — every one of the 19 lines mirrors around the center column
  (first+last non-space = 36); star redesigned into a real 5-pointer.
- ✅ **Runs without crashing** (3 s smoke run).
- ⏳ **Live visual/typing behavior** (smoothness, no flicker, help/exit/echo, bounce, the limit
  thresholds) — please confirm by running it in VS Code; this environment has no interactive TTY,
  so I could not watch the animation or type into it.

## 8. Notes / future
- Layout auto-clamps to the console size; for the intended look, keep the terminal ≥ ~90×30.
- `report-util`, `screen`, schedulers, etc. from the reference are intentionally **not** included —
  out of scope for this marquee task (kept simple per the rubric: animation + polling).
