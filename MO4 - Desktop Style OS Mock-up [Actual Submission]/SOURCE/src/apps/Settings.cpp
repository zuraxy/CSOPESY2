#include "Settings.h"

#include "imgui.h"

namespace Settings {

void Render(OsState& state) {
    ImGui::SetNextWindowSize(ImVec2(480, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(420, 140), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Settings", &state.showSettings)) {
        ImGui::End();
        return;
    }

    // Placeholder controls — values are dummies and affect nothing.
    ImGui::SeparatorText("Display");
    static float brightness = 80.0f;
    ImGui::SliderFloat("Brightness", &brightness, 0.0f, 100.0f, "%.0f%%");
    static int resolution = 0;
    const char* resolutions[] = { "1920 x 1080", "1600 x 900", "1366 x 768", "1280 x 720" };
    ImGui::Combo("Resolution", &resolution, resolutions, IM_ARRAYSIZE(resolutions));
    static int scale = 1;
    const char* scales[] = { "100%", "125%", "150%" };
    ImGui::Combo("Scale", &scale, scales, IM_ARRAYSIZE(scales));

    ImGui::SeparatorText("System");
    static bool notifications = true;
    static bool darkMode = true;
    static bool autoUpdates = false;
    ImGui::Checkbox("Enable notifications", &notifications);
    ImGui::Checkbox("Dark mode", &darkMode);
    ImGui::Checkbox("Automatic updates", &autoUpdates);

    ImGui::SeparatorText("Sound");
    static float volume = 65.0f;
    ImGui::SliderFloat("Volume", &volume, 0.0f, 100.0f, "%.0f%%");

    ImGui::SeparatorText("About");
    ImGui::Text("CSOPESY OS v1.0");
    ImGui::Text("A desktop-style OS mockup built with Dear ImGui.");
    ImGui::Spacing();
    ImGui::Text("Group Members:");
    ImGui::BulletText("Karl Matthew");
    ImGui::BulletText("Member 2");
    ImGui::BulletText("Member 3");
    ImGui::BulletText("Member 4");

    ImGui::End();
}

} // namespace Settings
