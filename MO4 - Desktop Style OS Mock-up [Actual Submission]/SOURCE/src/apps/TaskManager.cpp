#include "TaskManager.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "imgui.h"

namespace TaskManager {

struct Process {
    const char* name;
    int         pid;
    const char* status;
    float       cpu;    // percent
    float       memMb;  // megabytes
};

enum Column { ColName, ColPid, ColStatus, ColCpu, ColMem };

static std::vector<Process> g_processes = {
    { "System",                4,    "Running",   0.5f,  148.2f },
    { "csopesy_shell.exe",     312,  "Running",   2.1f,   86.4f },
    { "imgui_compositor.exe",  528,  "Running",   6.8f,  212.7f },
    { "explorer.exe",          1044, "Running",   1.3f,  154.9f },
    { "settings.exe",          1280, "Running",   0.4f,   48.3f },
    { "taskmgr.exe",           1532, "Running",   0.9f,   62.1f },
    { "svchost.exe",           1720, "Running",   0.2f,   34.5f },
    { "dwm.exe",               1988, "Running",   3.6f,  178.0f },
    { "audiosrv.exe",          2204, "Suspended", 0.0f,   21.8f },
    { "antivirus.exe",         2456, "Running",   4.2f,  301.6f },
    { "updater.exe",           2730, "Suspended", 0.0f,   17.2f },
    { "browser.exe",           2944, "Running",   8.7f,  642.3f },
};

static void SortProcesses(const ImGuiTableSortSpecs* specs) {
    if (specs->SpecsCount == 0) return;
    const ImGuiTableColumnSortSpecs& s = specs->Specs[0];
    const bool asc = (s.SortDirection == ImGuiSortDirection_Ascending);

    std::sort(g_processes.begin(), g_processes.end(),
              [&](const Process& a, const Process& b) {
        int cmp = 0;
        switch (s.ColumnUserID) {
            case ColName:   cmp = std::strcmp(a.name, b.name); break;
            case ColPid:    cmp = (a.pid < b.pid) ? -1 : (a.pid > b.pid) ? 1 : 0; break;
            case ColStatus: cmp = std::strcmp(a.status, b.status); break;
            case ColCpu:    cmp = (a.cpu < b.cpu) ? -1 : (a.cpu > b.cpu) ? 1 : 0; break;
            case ColMem:    cmp = (a.memMb < b.memMb) ? -1 : (a.memMb > b.memMb) ? 1 : 0; break;
        }
        return asc ? (cmp < 0) : (cmp > 0);
    });
}

void Render(OsState& state) {
    ImGui::SetNextWindowSize(ImVec2(620, 420), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(300, 160), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Task Manager", &state.showTaskManager)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Processes");
    ImGui::Separator();

    const ImGuiTableFlags flags =
        ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_Sortable |
        ImGuiTableFlags_Resizable;

    const float footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetTextLineHeightWithSpacing();
    if (ImGui::BeginTable("ProcessTable", 5, flags, ImVec2(0.0f, -footerHeight))) {
        ImGui::TableSetupColumn("Name",        ImGuiTableColumnFlags_WidthStretch, 0.0f, ColName);
        ImGui::TableSetupColumn("PID",         ImGuiTableColumnFlags_WidthFixed, 70.0f, ColPid);
        ImGui::TableSetupColumn("Status",      ImGuiTableColumnFlags_WidthFixed, 100.0f, ColStatus);
        ImGui::TableSetupColumn("CPU %",       ImGuiTableColumnFlags_WidthFixed, 80.0f, ColCpu);
        ImGui::TableSetupColumn("Memory (MB)", ImGuiTableColumnFlags_WidthFixed, 110.0f, ColMem);
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        if (ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs()) {
            if (sortSpecs->SpecsDirty) {
                SortProcesses(sortSpecs);
                sortSpecs->SpecsDirty = false;
            }
        }

        for (const Process& p : g_processes) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(p.name);
            ImGui::TableSetColumnIndex(1); ImGui::Text("%d", p.pid);
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(p.status);
            ImGui::TableSetColumnIndex(3); ImGui::Text("%.1f", p.cpu);
            ImGui::TableSetColumnIndex(4); ImGui::Text("%.1f", p.memMb);
        }
        ImGui::EndTable();
    }

    ImGui::Text("CPU: 23%%    Memory: 41%%    Processes: %d", (int)g_processes.size());
    ImGui::BeginDisabled();
    ImGui::Button("End task");
    ImGui::EndDisabled();

    ImGui::End();
}

} // namespace TaskManager
