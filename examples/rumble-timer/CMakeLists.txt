cmake_minimum_required(VERSION 3.20)
project(RumbleTimerPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

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
