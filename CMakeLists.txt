cmake_minimum_required(VERSION 3.20)
project(RLMod VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch ImGui ────────────────────────────────────────────────────────────────
include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

# ── Main executable ────────────────────────────────────────────────────────────
add_executable(RLMod WIN32
    src/main.cpp
    src/overlay.cpp
    src/console.cpp
    src/plugin_manager.cpp
    src/game_memory.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(RLMod PRIVATE
    src
    include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(RLMod PRIVATE
    d3d11
    dxgi
    dwmapi
    user32
    gdi32
)

target_compile_definitions(RLMod PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# MSVC: enable warnings, multiprocess build
if(MSVC)
    target_compile_options(RLMod PRIVATE /W3 /MP)
endif()

# ── Plugin SDK (header-only, used by plugin DLLs) ─────────────────────────────
add_library(RLModPluginSDK INTERFACE)
target_include_directories(RLModPluginSDK INTERFACE include)

# ── Example plugins ───────────────────────────────────────────────────────────
add_subdirectory(examples/hello-plugin)
add_subdirectory(examples/rumble-timer)
