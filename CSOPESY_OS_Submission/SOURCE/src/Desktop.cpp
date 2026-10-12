#include "Desktop.h"

#include <cstdio>
#include <ctime>

#include "imgui.h"

namespace Desktop {

static void DrawWallpaper(ImDrawList* draw, const ImVec2& pos, const ImVec2& size) {
    const ImU32 top    = IM_COL32(12, 24, 58, 255);   // deep blue
    const ImU32 bottom = IM_COL32(10, 90, 110, 255);  // teal
    draw->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y),
                                  top, top, bottom, bottom);

    // Soft translucent circles for depth.
    draw->AddCircleFilled(ImVec2(pos.x + size.x * 0.80f, pos.y + size.y * 0.25f),
                          size.y * 0.30f, IM_COL32(255, 255, 255, 8), 64);
    draw->AddCircleFilled(ImVec2(pos.x + size.x * 0.15f, pos.y + size.y * 0.75f),
                          size.y * 0.40f, IM_COL32(80, 180, 255, 10), 64);
    draw->AddCircleFilled(ImVec2(pos.x + size.x * 0.50f, pos.y + size.y * 0.55f),
                          size.y * 0.18f, IM_COL32(0, 255, 200, 7), 64);
}

static void DrawClock(ImDrawList* draw, const ImVec2& pos, const ImVec2& size) {
    std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);

    char timeText[16];
    char dateText[64];
    std::snprintf(timeText, sizeof(timeText), "%02d:%02d:%02d",
                  local.tm_hour, local.tm_min, local.tm_sec);
    std::strftime(dateText, sizeof(dateText), "%A, %B %d, %Y", &local);

    ImFont* font = ImGui::GetFont();
    const float timeSize = ImGui::GetFontSize() * 2.6f;
    const float dateSize = ImGui::GetFontSize() * 1.2f;

    const ImVec2 timeDim = font->CalcTextSizeA(timeSize, FLT_MAX, 0.0f, timeText);
    const ImVec2 dateDim = font->CalcTextSizeA(dateSize, FLT_MAX, 0.0f, dateText);

    const float margin = 24.0f;
    const ImVec2 timePos(pos.x + size.x - timeDim.x - margin, pos.y + margin);
    const ImVec2 datePos(pos.x + size.x - dateDim.x - margin, timePos.y + timeDim.y + 4.0f);

    // Subtle shadow for readability over the wallpaper.
    draw->AddText(font, timeSize, ImVec2(timePos.x + 2, timePos.y + 2), IM_COL32(0, 0, 0, 120), timeText);
    draw->AddText(font, timeSize, timePos, IM_COL32(255, 255, 255, 235), timeText);
    draw->AddText(font, dateSize, ImVec2(datePos.x + 1, datePos.y + 1), IM_COL32(0, 0, 0, 120), dateText);
    draw->AddText(font, dateSize, datePos, IM_COL32(255, 255, 255, 200), dateText);
}

static void DrawPowerButton(OsState& state, const ImVec2& size) {
    const float margin = 24.0f;
    const ImVec2 btnSize(64.0f, 32.0f);
    ImGui::SetCursorPos(ImVec2(size.x - btnSize.x - margin, 110.0f));

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.70f, 0.15f, 0.15f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.20f, 0.20f, 1.00f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.60f, 0.10f, 0.10f, 1.00f));
    if (ImGui::Button("PWR", btnSize)) {
        ImGui::OpenPopup("Shut Down?");
    }
    ImGui::PopStyleColor(3);

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    if (ImGui::BeginPopupModal("Shut Down?", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
        ImGui::Text("Shut down CSOPESY OS?");
        ImGui::Separator();
        if (ImGui::Button("Shut Down", ImVec2(120, 0))) {
            state.shutdownRequested = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Render(OsState& state) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::Begin("##Desktop", nullptr, flags);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    DrawWallpaper(draw, viewport->Pos, viewport->Size);
    DrawClock(draw, viewport->Pos, viewport->Size);
    DrawPowerButton(state, viewport->Size);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace Desktop
