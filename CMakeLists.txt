cmake_minimum_required(VERSION 3.24)
project(playtester C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Single-config generators (Makefiles, Ninja) build optimized code by default.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

include(FetchContent)

# ── SDL3: window, input and the renderer ImGui draws with ─────────────────────
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON  CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    sdl3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.4.20
    GIT_SHALLOW    TRUE
)

# ── Dear ImGui: the tool's user interface ─────────────────────────────────────
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.93.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(sdl3 imgui)

add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui_lib PUBLIC SDL3::SDL3-static)

# ── LevelScript: Bojan's generator, from the submodule (pinned commit) ────────
add_subdirectory(external/levelscript/src)   # defines ls_lib

# ── The tool ──────────────────────────────────────────────────────────────────
add_executable(playtester
    src/main.cpp
    src/level_source.cpp
    src/map.cpp
    src/map_view.cpp
)
target_link_libraries(playtester PRIVATE ls_lib imgui_lib)
target_compile_definitions(playtester PRIVATE
    DEFAULT_SCRIPT="${CMAKE_SOURCE_DIR}/scripts/roguelike.lvs")
if(MSVC)
    target_compile_options(playtester PRIVATE /W4)
else()
    target_compile_options(playtester PRIVATE -Wall -Wextra)
endif()
