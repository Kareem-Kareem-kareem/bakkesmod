# Built as a subdirectory of the main project.
# imgui is already fetched by the root CMakeLists — reuse it.

add_library(RumbleTimerPlugin SHARED
    rumble_timer_plugin.cpp
)

target_include_directories(RumbleTimerPlugin PRIVATE
    ../../include
    ${imgui_SOURCE_DIR}
)

target_compile_definitions(RumbleTimerPlugin PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

if(MSVC)
    target_compile_options(RumbleTimerPlugin PRIVATE /W3 /MP)
endif()
