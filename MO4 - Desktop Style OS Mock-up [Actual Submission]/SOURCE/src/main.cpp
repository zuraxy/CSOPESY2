// CSOPESY OS — desktop-style OS mockup.
// Compositor loop: Desktop (base layer) -> app windows -> Taskbar (top).

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

#include <cstdio>

#include "BootScreen.h"
#include "Desktop.h"
#include "OsState.h"
#include "Taskbar.h"
#include "Theme.h"
#include "apps/FileExplorer.h"
#include "apps/Settings.h"
#include "apps/TaskManager.h"

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialize GLFW\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "CSOPESY OS", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // vsync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // fresh layout every launch
    Theme::Apply();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    OsState state;

    while (!state.shutdownRequested) {
        glfwPollEvents();

        // Spec: app must close only via the PWR button — swallow the X button.
        if (glfwWindowShouldClose(window)) {
            glfwSetWindowShouldClose(window, GLFW_FALSE);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (state.bootPhase != OsState::BootPhase::Done) {
            BootScreen::Render(state);
        } else {
            Desktop::Render(state);
            if (state.showFileExplorer) FileExplorer::Render(state);
            if (state.showSettings)     Settings::Render(state);
            if (state.showTaskManager)  TaskManager::Render(state);
            Taskbar::Render(state);
        }

        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
