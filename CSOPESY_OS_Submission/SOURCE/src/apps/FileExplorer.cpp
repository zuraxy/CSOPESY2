#include "FileExplorer.h"

#include "imgui.h"

namespace FileExplorer {

struct FileEntry {
    const char* name;
    const char* type;
    const char* size;
    const char* modified;
};

static const FileEntry kFiles[] = {
    { "Documents",          "File folder",   "--",      "06/01/2026 09:12 AM" },
    { "Pictures",           "File folder",   "--",      "05/28/2026 04:45 PM" },
    { "Downloads",          "File folder",   "--",      "06/10/2026 11:03 PM" },
    { "MCO_Report.docx",    "Word Document", "248 KB",  "06/09/2026 08:31 PM" },
    { "csopesy_notes.txt",  "Text Document", "12 KB",   "06/07/2026 02:14 PM" },
    { "presentation.pptx",  "PowerPoint",    "3.1 MB",  "06/10/2026 10:47 PM" },
    { "wallpaper.png",      "PNG Image",     "1.8 MB",  "05/20/2026 06:22 PM" },
    { "main.cpp",           "C++ Source",    "9 KB",    "06/11/2026 01:05 AM" },
    { "build.log",          "Log File",      "84 KB",   "06/11/2026 01:06 AM" },
};

void Render(OsState& state) {
    ImGui::SetNextWindowSize(ImVec2(640, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(200, 120), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("File Explorer", &state.showFileExplorer)) {
        ImGui::End();
        return;
    }

    // Fake navigation bar (placeholder, non-functional).
    ImGui::Button("<");
    ImGui::SameLine();
    ImGui::Button(">");
    ImGui::SameLine();
    static char path[128] = "C:\\Users\\Group\\Documents";
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##path", path, sizeof(path), ImGuiInputTextFlags_ReadOnly);

    ImGui::Separator();

    const ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("FileTable", 4, flags)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Date modified", ImGuiTableColumnFlags_WidthFixed, 160.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (const FileEntry& f : kFiles) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Selectable(f.name, false, ImGuiSelectableFlags_SpanAllColumns);
            ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(f.type);
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(f.size);
            ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(f.modified);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

} // namespace FileExplorer
