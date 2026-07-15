# Built as a subdirectory of the main project.
# imgui is already fetched by the root CMakeLists — reuse it.

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
