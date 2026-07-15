cmake_minimum_required(VERSION 3.20)
project(HelloPlugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch ImGui (same version as the host — must match at runtime)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

add_library(HelloPlugin SHARED
    hello_plugin.cpp
)

target_include_directories(HelloPlugin PRIVATE
    ../../include
    ${imgui_SOURCE_DIR}
)

target_compile_definitions(HelloPlugin PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN)

if(MSVC)
    target_compile_options(HelloPlugin PRIVATE /W3 /MP)
endif()
