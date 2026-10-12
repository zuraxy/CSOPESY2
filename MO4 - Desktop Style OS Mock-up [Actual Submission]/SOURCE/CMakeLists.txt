cmake_minimum_required(VERSION 3.16)
project(csopesy_os LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
)

FetchContent_MakeAvailable(glfw imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw)

add_executable(csopesy_os
    src/main.cpp
    src/Theme.cpp
    src/BootScreen.cpp
    src/Desktop.cpp
    src/Taskbar.cpp
    src/apps/TaskManager.cpp
    src/apps/FileExplorer.cpp
    src/apps/Settings.cpp
)

target_include_directories(csopesy_os PRIVATE src)
target_link_libraries(csopesy_os PRIVATE imgui glfw opengl32)

# Bundle the GCC runtime so the Windows executable can run without MSYS2.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND WIN32)
    target_link_options(csopesy_os PRIVATE -static)
endif()
