cmake_minimum_required(VERSION 3.10)
project(game)

set(CMAKE_CXX_STANDARD 20)
include(CTest)

option(
    GAME_REVIEW_BUILD
    "Enable debug tools while preserving the normal title-screen startup"
    OFF
)

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/src/thirdParty/.*")

add_executable(game ${GAME_SOURCES})

add_executable(editor_restart_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/editor_restart_helper/Main.cpp
)

if (MSVC)
    target_compile_options(game PRIVATE /utf-8 /MP)
    target_compile_options(editor_restart_helper PRIVATE /utf-8)
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

if (BUILD_TESTING)
    add_executable(game_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageActorNodeFactoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StagePlatformIdentifiersTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StagePlatformConnectionsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageYamlRepositoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCWorkFileNameTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCWorkStorageTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPlatformGridTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPlatformDocumentTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageEditHistoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCWorkMetadataTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCSessionStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPreviewStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCEditorStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCClearTransitionStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPresetVisualsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCEditorTutorialTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageProgressSystemTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerConfigLoaderTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/EnemyConfigLoaderTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerControlConfigurationStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/FramePerformanceTrackerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCModeControllerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlatformStageConfigTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerCameraSettingsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/CameraShakeEffectTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TutorialObjectiveTextTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/OperationGuideVisibilityTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerJewelGaugeTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/StageActorNodeFactory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/StagePlatformIdentifiers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/StagePlatformConnections.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/StageYamlRepositoryOperations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCWorkFileName.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCWorkStorage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/UGCPlatformGrid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/UGCPlatformDocument.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/StageEditHistory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCWorkMetadata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCSessionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCPreviewState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCEditorToolState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCEditorMenuState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCPreviewPanelState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCSelectionDragState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCSwitchConnectionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCClearTransitionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/stage/UGCPresetVisuals.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/debug/ugc/UGCEditorTutorial.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/StageProgressSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/UserDataPaths.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/player/PlayerConfigLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/enemy/EnemyConfigLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/PlayerControlConfigurationState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/performance/FramePerformanceTracker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/UGCModeController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/actor_loader/PlatformStageConfig.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/camera/PlayerCameraSettings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/camera/CameraShakeEffect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/tutorial/TutorialLibrary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/text/JapaneseRubyGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/ui/OperationGuideVisibility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/player/PlayerJewelGauge.cpp
    )
    target_include_directories(game_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )
    target_link_libraries(game_tests PRIVATE
        glm::glm
        yaml-cpp::yaml-cpp
    )
    if (MSVC)
        target_compile_options(game_tests PRIVATE /utf-8 /MP)
    endif()
    if (WIN32)
        target_link_libraries(game_tests PRIVATE windowsapp)
    endif()
    add_test(NAME game_tests COMMAND game_tests)
endif()

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
    yaml-cpp::yaml-cpp
)

if (WIN32)
    target_link_libraries(game
        windowsapp
        mfplat
        mfreadwrite
        mfuuid
        ole32
    )

    add_custom_target(package_windows
        COMMAND powershell
            -NoProfile
            -ExecutionPolicy Bypass
            -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/Create-WindowsDistribution.ps1
            -BuildDirectory ${CMAKE_BINARY_DIR}
            -Configuration $<CONFIG>
            -OutputDirectory ${CMAKE_CURRENT_SOURCE_DIR}/dist
            -SkipBuild
        DEPENDS game
        USES_TERMINAL
    )

    if (GAME_REVIEW_BUILD)
        add_custom_target(package_windows_review
            COMMAND powershell
                -NoProfile
                -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/Create-WindowsDistribution.ps1
                -BuildDirectory ${CMAKE_BINARY_DIR}
                -Configuration $<CONFIG>
                -OutputDirectory ${CMAKE_CURRENT_SOURCE_DIR}/dist-review
                -SkipBuild
            DEPENDS game
            USES_TERMINAL
        )
    endif()

endif()

if (APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 20)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
    set(TUTORIAL_VIDEO_MAC_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/video/TutorialVideoPlayerMacBackend.mm
    )
    target_sources(game PRIVATE ${TUTORIAL_VIDEO_MAC_SOURCE})
    set_source_files_properties(
        ${TUTORIAL_VIDEO_MAC_SOURCE}
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )

    find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
    find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    target_link_libraries(game
        ${AVFOUNDATION_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()

set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(editor_restart_helper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_definitions(game PRIVATE SDL_MAIN_HANDLED)
if (GAME_REVIEW_BUILD)
    target_compile_definitions(game PRIVATE GAME_REVIEW_BUILD=1)
endif()
