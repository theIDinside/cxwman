cmake_minimum_required(VERSION 3.17)
project(cxwman)

set(CMAKE_CXX_STANDARD 17)
set(EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/bin/release)
if(MY_WARNINGS STREQUAL "on")
	set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wcast-align -Wunused -Wpedantic -Wconversion -Wsign-conversion -Wdouble-promotion")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_RELEASE} -O3")

include(FetchContent)

FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 6.2.1
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/dep/3rdparty/fmt)

FetchContent_MakeAvailable(fmt)

if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
    add_subdirectory(${fmt_SOURCE_DIR} ${fmt_BINARY_DIR})
endif()

set(SOURCES src/main.cpp			src/xcom/connect.cpp src/datastructure/geometry.cpp src/xcom/window.cpp src/xcom/workspace.cpp src/xcom/container.cpp src/xcom/utility/event_table.cpp)
set(HEADERS src/coreutils/core.hpp	src/xcom/connect.hpp src/datastructure/geometry.hpp src/xcom/window.hpp src/xcom/workspace.hpp src/xcom/container.hpp src/xcom/utility/event_table.hpp)


add_executable(cxwman ${SOURCES} ${HEADERS})
target_include_directories(cxwman PRIVATE ./src)
target_link_libraries(cxwman xcb fmt::fmt xcb-keysyms)

add_executable(xcb_test tests/xcb_test.cpp)
target_link_libraries(xcb_test xcb)

add_executable(window_evt_gfx tests/window_event_graphics.cpp)
target_link_libraries(window_evt_gfx xcb)

add_executable(xcb_glx tests/xcb_glx.cpp)
target_link_libraries(xcb_glx xcb GL X11 X11-xcb)



if(CMAKE_BUILD_TYPE STREQUAL Release)
    message("Build type is ${CMAKE_BUILD_TYPE}. Copying assets to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}")
    message("Release has instrumentation features disabled. To enable instrumentation for Release mode, build with type ReleaseInst.")
	add_custom_command(TARGET cxwman PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/misc ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
else()
    message("Build type is ${CMAKE_BUILD_TYPE}. Copying assets to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}")
    message("Debug has instrumentation features enabled.")
	target_compile_definitions(cxwman PUBLIC INSTRUMENTATION_SET KEEP_LOGS DEBUGGING)
	add_custom_command(TARGET cxwman PRE_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${PROJECT_SOURCE_DIR}/misc ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

				add_custom_command(TARGET cxwman POST_BUILD	COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --green "Built with DEBUGGING")
endif()
