cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/src/thirdParty/.*")

add_executable(game ${GAME_SOURCES})

if (MSVC)
    target_compile_options(game PRIVATE /utf-8)
endif()

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdParty/imgui)
set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdParty/ImGuizmo/src)

target_sources(game PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp

    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp

    ${IMGUIZMO_DIR}/ImGuizmo.cpp
)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet REQUIRED)
find_package(SDL2 REQUIRED CONFIG)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)
find_package(yaml-cpp REQUIRED)

target_include_directories(game PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
)
target_link_directories(game PRIVATE /opt/homebrew/lib)

target_link_libraries(game
    GLEW::GLEW
    glfw
    OpenGL::GL
    glm::glm
    SDL2::SDL2
    assimp::assimp
    ${BULLET_LIBRARIES}
    SDL2_mixer::SDL2_mixer
    SDL2_ttf::SDL2_ttf
    yaml-cpp
)

set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_definitions(game PRIVATE SDL_MAIN_HANDLED)