#include "Taskbar.h"

#include <cmath>

#include "imgui.h"

namespace Taskbar {

static const float kHeight = 48.0f;

enum class Icon { Folder, Gear, Bars };

// ---- Hand-drawn glyphs (ImDrawList primitives) ------------------------------
// Each draws into a square box centered at `c` with half-extent `s`.

static const ImU32 kIconCol = IM_COL32(225, 235, 250, 255);

static void DrawFolderIcon(ImDrawList* d, const ImVec2& c, float s) {
    // Tab on top-left, then the folder body.
    d->AddRectFilled(ImVec2(c.x - s, c.y - s * 0.55f),
                     ImVec2(c.x - s * 0.1f, c.y - s * 0.2f), kIconCol, 2.0f);
    d->AddRectFilled(ImVec2(c.x - s, c.y - s * 0.25f),
                     ImVec2(c.x + s, c.y + s * 0.6f), kIconCol, 2.0f);
}

static void DrawGearIcon(ImDrawList* d, const ImVec2& c, float s) {
    const float r = s * 0.7f;
    for (int i = 0; i < 6; ++i) {                 // teeth
        float a = (float)i / 6.0f * 6.2831853f;
        ImVec2 in(c.x + cosf(a) * r, c.y + sinf(a) * r);
        ImVec2 out(c.x + cosf(a) * s, c.y + sinf(a) * s);
        d->AddLine(in, out, kIconCol, 3.0f);
    }
    d->AddCircle(c, r, kIconCol, 16, 2.5f);       // ring
    d->AddCircleFilled(c, r * 0.35f, kIconCol, 12); // hub
}

static void DrawBarsIcon(ImDrawList* d, const ImVec2& c, float s) {
    const float w = s * 0.42f;                    // bar width
    const float bot = c.y + s * 0.7f;
    const float xs[3] = { c.x - s * 0.75f, c.x, c.x + s * 0.75f };
    const float hs[3] = { s * 0.7f, s * 1.4f, s * 1.0f };
    for (int i = 0; i < 3; ++i) {
        d->AddRectFilled(ImVec2(xs[i] - w * 0.5f, bot - hs[i]),
                         ImVec2(xs[i] + w * 0.5f, bot), kIconCol, 1.0f);
    }
}

static void DrawIcon(ImDrawList* d, Icon icon, const ImVec2& c, float s) {
    switch (icon) {
        case Icon::Folder: DrawFolderIcon(d, c, s); break;
        case Icon::Gear:   DrawGearIcon(d, c, s);   break;
        case Icon::Bars:   DrawBarsIcon(d, c, s);   break;
    }
}

// Taskbar button: drawn icon at the left, name (rendered by ImGui via the
// leading-space gap in `label`) at the right. Toggles an app's open flag and
// shows an accent underline while running.
static void AppButton(const char* label, Icon icon, bool& openFlag) {
    // Capture before the Button click can toggle openFlag, so the push and
    // pop below always pair up (otherwise a toggling click unbalances the
    // style-color stack and trips an ImGui assert).
    const bool wasOpen = openFlag;
    if (wasOpen) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.30f, 0.45f, 1.00f));
    }
    if (ImGui::Button(label, ImVec2(120.0f, kHeight - 14.0f))) {
        openFlag = !openFlag;
    }
    if (wasOpen) {
        ImGui::PopStyleColor();
    }

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Icon sits in the left gap reserved by the label's leading spaces.
    const float s = (max.y - min.y) * 0.32f;
    DrawIcon(draw, icon, ImVec2(min.x + 16.0f, (min.y + max.y) * 0.5f), s);

    if (wasOpen) {
        draw->AddRectFilled(ImVec2(min.x + 8.0f, max.y - 3.0f),
                            ImVec2(max.x - 8.0f, max.y),
                            IM_COL32(70, 160, 255, 255), 2.0f);
    }
    ImGui::SameLine();
}

void Render(OsState& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + viewport->Size.y - kHeight));
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, kHeight));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 7.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.07f, 0.09f, 0.97f));

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("##Taskbar", nullptr, flags);

    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.30f, 0.65f, 1.00f, 1.00f), "CSOPESY OS");
    ImGui::SameLine(0.0f, 24.0f);

    AppButton("    Files",        Icon::Folder, state.showFileExplorer);
    AppButton("    Settings",     Icon::Gear,   state.showSettings);
    AppButton("    Task Manager", Icon::Bars,   state.showTaskManager);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace Taskbar
