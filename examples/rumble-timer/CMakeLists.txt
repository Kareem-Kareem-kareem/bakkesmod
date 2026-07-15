# Built as a subdirectory of the main project.
# imgui is already fetched by the root CMakeLists — reuse imgui_SOURCE_DIR.
# Each plugin DLL must compile ImGui itself; it cannot share symbols with RLMod.exe.

add_library(RumbleTimerPlugin SHARED
    rumble_timer_plugin.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(RumbleTimerPlugin PRIVATE
    ../../include
    ${imgui_SOURCE_DIR}
)

target_compile_definitions(RumbleTimerPlugin PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

if(MSVC)
    target_compile_options(RumbleTimerPlugin PRIVATE /W3 /MP)
endif()
