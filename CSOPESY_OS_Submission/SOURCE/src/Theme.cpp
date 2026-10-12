#include "Theme.h"

#include "imgui.h"

namespace Theme {

void Apply() {
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding    = 8.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.GrabRounding      = 6.0f;
    style.TabRounding       = 6.0f;
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(8.0f, 5.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.WindowTitleAlign  = ImVec2(0.5f, 0.5f);
    style.WindowBorderSize  = 1.0f;

    ImVec4* colors = style.Colors;
    const ImVec4 bg       = ImVec4(0.09f, 0.10f, 0.13f, 1.00f);
    const ImVec4 bgLight  = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    const ImVec4 accent   = ImVec4(0.20f, 0.55f, 0.95f, 1.00f);
    const ImVec4 accentHi = ImVec4(0.30f, 0.65f, 1.00f, 1.00f);

    colors[ImGuiCol_WindowBg]          = ImVec4(bg.x, bg.y, bg.z, 0.98f);
    colors[ImGuiCol_PopupBg]           = ImVec4(bg.x, bg.y, bg.z, 0.98f);
    colors[ImGuiCol_TitleBg]           = bg;
    colors[ImGuiCol_TitleBgActive]     = ImVec4(0.12f, 0.20f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]  = bg;
    colors[ImGuiCol_FrameBg]           = bgLight;
    colors[ImGuiCol_FrameBgHovered]    = ImVec4(0.20f, 0.24f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]     = ImVec4(0.24f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_Button]            = bgLight;
    colors[ImGuiCol_ButtonHovered]     = accent;
    colors[ImGuiCol_ButtonActive]      = accentHi;
    colors[ImGuiCol_Header]            = ImVec4(accent.x, accent.y, accent.z, 0.35f);
    colors[ImGuiCol_HeaderHovered]     = ImVec4(accent.x, accent.y, accent.z, 0.55f);
    colors[ImGuiCol_HeaderActive]      = ImVec4(accent.x, accent.y, accent.z, 0.75f);
    colors[ImGuiCol_SliderGrab]        = accent;
    colors[ImGuiCol_SliderGrabActive]  = accentHi;
    colors[ImGuiCol_CheckMark]         = accentHi;
    colors[ImGuiCol_Separator]         = ImVec4(0.25f, 0.28f, 0.34f, 1.00f);
    colors[ImGuiCol_Border]            = ImVec4(0.25f, 0.28f, 0.34f, 0.60f);
    colors[ImGuiCol_Tab]               = bgLight;
    colors[ImGuiCol_TabHovered]        = accent;
    colors[ImGuiCol_TableHeaderBg]     = bgLight;
    colors[ImGuiCol_TableRowBg]        = ImVec4(1.0f, 1.0f, 1.0f, 0.02f);
    colors[ImGuiCol_TableRowBgAlt]     = ImVec4(1.0f, 1.0f, 1.0f, 0.05f);
    colors[ImGuiCol_ResizeGrip]        = ImVec4(accent.x, accent.y, accent.z, 0.30f);
    colors[ImGuiCol_ResizeGripHovered] = accent;
    colors[ImGuiCol_ResizeGripActive]  = accentHi;
}

} // namespace Theme
