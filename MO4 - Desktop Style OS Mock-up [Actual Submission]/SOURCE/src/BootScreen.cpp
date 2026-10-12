#include "BootScreen.h"

#include <algorithm>
#include <cstdio>

#include "imgui.h"

namespace BootScreen {

static const float kPostDur   = 3.0f;  // seconds of BIOS POST
static const float kSplashDur = 3.0f;  // seconds of OS splash

// ---- Phase 1: BIOS POST -----------------------------------------------------

static void RenderPost(ImDrawList* draw, const ImVec2& origin, const ImVec2& size,
                       float elapsed) {
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                        IM_COL32(0, 0, 0, 255));

    const ImU32 green = IM_COL32(51, 255, 102, 255);
    ImFont* font = ImGui::GetFont();
    const float fontSize = ImGui::GetFontSize() * 1.35f;
    const float lineH = fontSize * 1.35f;

    const float p = std::min(elapsed / kPostDur, 1.0f); // 0..1 over the phase
    const int   kLines = 6;

    // RAM line (index 2) counts up while it is the freshest revealed line.
    const float ramStart = 2.0f / kLines;
    const float ramEnd   = 3.0f / kLines;
    char ramText[48];
    if (p < ramEnd) {
        float f = std::clamp((p - ramStart) / (ramEnd - ramStart), 0.0f, 1.0f);
        std::snprintf(ramText, sizeof(ramText), "Checking RAM : %5dK",
                      static_cast<int>(f * 64000.0f));
    } else {
        std::snprintf(ramText, sizeof(ramText), "Checking RAM : 64000K OK");
    }

    const char* lines[kLines] = {
        "CSOPESY BIOS v1.0",
        "CPU TYPE     : Cavemann Core 64-bit @ 3.50GHz",
        ramText,
        "Detecting Storage ... OK",
        "Initializing Devices ... OK",
        "Starting CSOPESY OS ...",
    };

    ImVec2 cursor(origin.x + 40.0f, origin.y + 40.0f);
    for (int i = 0; i < kLines; ++i) {
        if (p < static_cast<float>(i) / kLines) break; // not revealed yet
        draw->AddText(font, fontSize, cursor, green, lines[i]);
        cursor.y += lineH;
    }
}

// ---- Phase 2: OS splash -----------------------------------------------------

static void RenderSplash(ImDrawList* draw, const ImVec2& origin, const ImVec2& size,
                         float elapsed) {
    draw->AddRectFilled(origin, ImVec2(origin.x + size.x, origin.y + size.y),
                        IM_COL32(12, 24, 58, 255));

    // Centered big title.
    ImFont* font = ImGui::GetFont();
    const float titleSize = ImGui::GetFontSize() * 3.2f;
    const char* title = "CSOPESY OS";
    const ImVec2 dim = font->CalcTextSizeA(titleSize, FLT_MAX, 0.0f, title);
    const ImVec2 center(origin.x + size.x * 0.5f, origin.y + size.y * 0.5f);
    draw->AddText(font, titleSize, ImVec2(center.x - dim.x * 0.5f, center.y - dim.y),
                  IM_COL32(255, 255, 255, 240), title);

    // Centered progress bar below the title.
    const float barW = 420.0f;
    const float frac = std::min(elapsed / kSplashDur, 1.0f);
    ImGui::SetCursorScreenPos(ImVec2(center.x - barW * 0.5f, center.y + 24.0f));
    ImGui::ProgressBar(frac, ImVec2(barW, 0.0f));

    const char* sub = "booting...";
    const float subSize = ImGui::GetFontSize() * 1.1f;
    const ImVec2 subDim = font->CalcTextSizeA(subSize, FLT_MAX, 0.0f, sub);
    draw->AddText(font, subSize, ImVec2(center.x - subDim.x * 0.5f, center.y + 60.0f),
                  IM_COL32(160, 180, 210, 220), sub);
}

void Render(OsState& state) {
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->Pos);
    ImGui::SetNextWindowSize(vp->Size);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##Boot", nullptr, flags);
    ImDrawList* draw = ImGui::GetWindowDrawList();

    state.bootElapsed += ImGui::GetIO().DeltaTime;

    if (state.bootPhase == OsState::BootPhase::Post) {
        RenderPost(draw, vp->Pos, vp->Size, state.bootElapsed);
        if (state.bootElapsed >= kPostDur) {
            state.bootPhase = OsState::BootPhase::Splash;
            state.bootElapsed = 0.0f;
        }
    } else { // Splash
        RenderSplash(draw, vp->Pos, vp->Size, state.bootElapsed);
        if (state.bootElapsed >= kSplashDur) {
            state.bootPhase = OsState::BootPhase::Done;
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

} // namespace BootScreen
