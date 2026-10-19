cmake_minimum_required(VERSION 3.13)
project(game)

set(CMAKE_CXX_STANDARD 20)
include(CTest)

option(GAME_ENABLE_TEST_SANITIZERS "Enable AddressSanitizer and UBSan for headless unit tests only" OFF)
option(GAME_ENABLE_RUNTIME_SANITIZERS "Enable AddressSanitizer and UBSan for the game and runtime integration tests" OFF)
option(GAME_ENABLE_WARNINGS "Enable additional compiler warnings in first-party sources" OFF)
option(GAME_ENABLE_COVERAGE "Instrument the game and tests for LLVM source coverage" OFF)
option(GAME_BUILD_BENCHMARKS "Build fixed-input CPU benchmarks that do not open a window" OFF)

option(
    GAME_REVIEW_BUILD
    "Enable debug tools while preserving the normal title-screen startup"
    OFF
)
option(
    GAME_TGS_BUILD
    "Build the self-contained ten-minute TGS exhibition version"
    OFF
)

if (GAME_REVIEW_BUILD AND GAME_TGS_BUILD)
    message(FATAL_ERROR "GAME_REVIEW_BUILD and GAME_TGS_BUILD cannot both be enabled")
endif()

file(GLOB_RECURSE GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

list(FILTER GAME_SOURCES EXCLUDE REGEX ".*/src/thirdParty/.*")

# エントリーポイントと保存先解決だけを差し替え、統合テストでも同じRuntimeを使う。
list(REMOVE_ITEM GAME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app/Main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/system/UserDataPaths.cpp"
)
add_library(game_runtime OBJECT ${GAME_SOURCES})
add_executable(game src/app/Main.cpp src/system/UserDataPaths.cpp)
target_link_libraries(game PRIVATE game_runtime)
if (GAME_ENABLE_RUNTIME_SANITIZERS)
    if (MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(FATAL_ERROR "GAME_ENABLE_RUNTIME_SANITIZERS requires GCC or Clang with a GNU-style command line")
    endif()
    # Runtime本体と利用側の両方を計装し、テストのラッパーだけに適用される状態を避ける。
    target_compile_options(game_runtime PUBLIC
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=undefined
    )
    target_link_options(game_runtime INTERFACE -fsanitize=address,undefined)
endif()
option(GAME_BUILD_RUNTIME_TESTS "Build integration tests requiring an OpenGL display" OFF)

add_executable(editor_restart_helper
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/editor_restart_helper/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/ProcessLaunch.cpp
)
target_include_directories(editor_restart_helper PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if (MSVC)
    target_compile_options(game_runtime PUBLIC /utf-8 /MP /FS)
    target_compile_options(editor_restart_helper PRIVATE /utf-8)
endif()

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdParty/imgui)
set(IMGUIZMO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/thirdParty/ImGuizmo/src)

target_sources(game_runtime PRIVATE
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
    add_executable(game_asset_check tools/asset_check/Main.cpp)
    target_link_libraries(game_asset_check PRIVATE yaml-cpp::yaml-cpp)
    if (MSVC)
        target_compile_options(game_asset_check PRIVATE /utf-8)
    endif()
    add_executable(game_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/EditorReliabilityTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StagePickPolicy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ui/UICanvasEditHistory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TestMain.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/YamlReadTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/ConfigurationPersistenceTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/tools/sequence/StorybookEditorPersistence.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/session/EditorSessionRepository.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/enemy/EnemyPresetRepository.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/effect/particle/ParticleConfigLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/story/StorybookConfig.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/ending/EndingRollConfig.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PersistenceSafetyTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/NPCTalkDataTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/NPCTalkData.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageActorNodeFactoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StagePlatformIdentifiersTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StagePlatformConnectionsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageYamlRepositoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCWorkFileNameTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCWorkStorageTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPlatformGridTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCPlatformDocumentTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageEditHistoryTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/StageActorSaveStateTests.cpp
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
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/EnemyGuardGaugeTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerControlConfigurationStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerInputDeviceRoutingTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/FramePerformanceTrackerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/UGCModeControllerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlatformStageConfigTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerCameraSettingsTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/CameraShakeEffectTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TutorialObjectiveTextTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/OperationGuideVisibilityTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerJewelGaugeTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/PlayerSplitGuardStateTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/TgsExperienceControllerTests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StageActorNodeFactory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StagePlatformIdentifiers.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StagePlatformConnections.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StageYamlRepositoryOperations.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCWorkFileName.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCWorkStorage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/UGCPlatformGrid.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/UGCPlatformDocument.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StageEditHistory.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StageActorSaveState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCWorkMetadata.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCSessionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCPreviewState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCEditorToolState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCEditorMenuState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCPreviewPanelState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCSelectionDragState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCSwitchConnectionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gameplay/ugc/UGCClearTransitionState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/UGCPresetVisuals.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/ugc/UGCEditorTutorial.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/StageProgressSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/UserDataPaths.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/player/PlayerConfigLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/enemy/EnemyConfigLoader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/enemy/EnemyGuardGauge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/PlayerControlConfigurationState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/PlayerInputDeviceRouting.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/performance/FramePerformanceTracker.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/UGCModeController.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/actor_loader/PlatformStageConfig.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/camera/PlayerCameraSettings.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/camera/CameraShakeEffect.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/AtomicFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TextFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/YamlFile.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/editor/stage/StageEditTransaction.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/camera/CinematicSequenceLibrary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/sequence/SequenceLibrary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/UILoadSystem.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/tutorial/TutorialLibrary.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/text/JapaneseRubyGenerator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/gfx/ui/OperationGuideVisibility.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/actor/player/PlayerJewelGauge.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/PlayerSplitGuardState.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/system/tgs/TgsExperienceController.cpp
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
        target_compile_options(game_tests PRIVATE /utf-8 /MP /FS)
    endif()
    if (WIN32)
        target_link_libraries(game_tests PRIVATE windowsapp)
    endif()
    if (GAME_ENABLE_TEST_SANITIZERS)
        if (MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            message(FATAL_ERROR "GAME_ENABLE_TEST_SANITIZERS requires GCC or Clang with a GNU-style command line")
        endif()
        target_compile_options(game_tests PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
            -fno-sanitize-recover=undefined
        )
        target_link_options(game_tests PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME game_tests COMMAND game_tests)

    find_package(Python3 QUIET COMPONENTS Interpreter)
    if (Python3_Interpreter_FOUND)
        add_test(NAME asset_schema_tests
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tools/validate_assets.py
                --validator $<TARGET_FILE:game_asset_check>
        )
        add_test(NAME asset_validation_tests
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/AssetValidationTests.py
                --validator $<TARGET_FILE:game_asset_check>
        )
        add_executable(restart_launch_probe tests/RestartLaunchProbe.cpp)
        add_test(NAME restart_helper_tests
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/RestartHelperTests.py
                --helper $<TARGET_FILE:editor_restart_helper>
                --probe $<TARGET_FILE:restart_launch_probe>
                --cmake ${CMAKE_COMMAND}
        )
        set_tests_properties(restart_helper_tests PROPERTIES TIMEOUT 60)
        add_test(NAME architecture_audit_tests
            COMMAND ${Python3_EXECUTABLE} -B -m unittest discover -s tests -p ArchitectureAuditTests.py
        )
        set_tests_properties(architecture_audit_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
endif()

target_include_directories(game_runtime PUBLIC
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUIZMO_DIR}
)
target_include_directories(game_runtime SYSTEM PUBLIC ${BULLET_INCLUDE_DIRS})

target_link_libraries(game_runtime PUBLIC
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
    target_link_libraries(game_runtime PUBLIC
        windowsapp
        mfplat
        mfreadwrite
        mfuuid
        ole32
        imm32
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

    if (GAME_TGS_BUILD)
        add_custom_target(package_windows_tgs
            COMMAND powershell
                -NoProfile
                -ExecutionPolicy Bypass
                -File ${CMAKE_CURRENT_SOURCE_DIR}/scripts/Create-WindowsDistribution.ps1
                -BuildDirectory ${CMAKE_BINARY_DIR}
                -Configuration $<CONFIG>
                -OutputDirectory ${CMAKE_CURRENT_SOURCE_DIR}/dist-tgs
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
    target_sources(game_runtime PRIVATE ${TUTORIAL_VIDEO_MAC_SOURCE})
    set_source_files_properties(
        ${TUTORIAL_VIDEO_MAC_SOURCE}
        PROPERTIES COMPILE_FLAGS "-fobjc-arc"
    )

    find_library(AVFOUNDATION_FRAMEWORK AVFoundation REQUIRED)
    find_library(COREMEDIA_FRAMEWORK CoreMedia REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
    target_link_libraries(game_runtime PUBLIC
        ${AVFOUNDATION_FRAMEWORK}
        ${COREMEDIA_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
endif()

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "${CMAKE_BINARY_DIR}/shaders"
    COMMENT "Copying runtime assets and shaders..."
)

set_target_properties(game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

set_target_properties(editor_restart_helper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_definitions(game_runtime PUBLIC SDL_MAIN_HANDLED)
if (GAME_REVIEW_BUILD)
    target_compile_definitions(game_runtime PUBLIC GAME_REVIEW_BUILD=1)
endif()
if (GAME_TGS_BUILD)
    target_compile_definitions(game_runtime PUBLIC GAME_TGS_BUILD=1)
endif()

if (BUILD_TESTING AND GAME_BUILD_RUNTIME_TESTS AND NOT GAME_TGS_BUILD)
    add_executable(game_runtime_tests
        tests/RuntimeIntegrationTests.cpp
        tests/RuntimeGameplayTests.cpp
        tests/RuntimeReliabilityTests.cpp
        tests/RuntimeEditorPersistenceTests.cpp
        tests/IsolatedUserDataPaths.cpp
    )
    target_link_libraries(game_runtime_tests PRIVATE game_runtime)
    target_include_directories(game_runtime_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime-tests")
    add_test(NAME game_runtime_tests COMMAND game_runtime_tests)
    set(GAME_RUNTIME_TEST_TIMEOUT 120)
    if (GAME_ENABLE_RUNTIME_SANITIZERS)
        # 全体計装ではYAML再構築を含む実行時間が伸びるため、通常構成とは制限を分ける。
        set(GAME_RUNTIME_TEST_TIMEOUT 300)
    endif()
    set_tests_properties(game_runtime_tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/runtime-tests"
        ENVIRONMENT "SDL_AUDIODRIVER=dummy"
        LABELS "integration;display"
        TIMEOUT ${GAME_RUNTIME_TEST_TIMEOUT}
        RUN_SERIAL TRUE
    )
    add_dependencies(game_runtime_tests game)
    if (Python3_Interpreter_FOUND)
        add_executable(game_startup_probe
            src/app/Main.cpp
            tests/StartupEntryProbe.cpp
            tests/IsolatedUserDataPaths.cpp
        )
        target_link_libraries(game_startup_probe PRIVATE game_runtime)
        target_compile_definitions(game_startup_probe PRIVATE main=RunProductionMain)
        add_test(NAME startup_exit_tests
            COMMAND ${Python3_EXECUTABLE} -B ${CMAKE_CURRENT_SOURCE_DIR}/tests/StartupExitTests.py
                --probe $<TARGET_FILE:game_startup_probe>
        )
        set_tests_properties(startup_exit_tests PROPERTIES TIMEOUT 30 LABELS "integration;display" RUN_SERIAL TRUE)
    endif()
endif()

if (GAME_BUILD_BENCHMARKS)
    add_executable(game_benchmarks tools/benchmarks/Main.cpp src/system/UserDataPaths.cpp)
    target_link_libraries(game_benchmarks PRIVATE game_runtime)
endif()

if (GAME_ENABLE_WARNINGS)
    file(GLOB GAME_TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp")
    set(GAME_WARNING_SOURCES ${GAME_SOURCES} ${GAME_TEST_SOURCES} ${TUTORIAL_VIDEO_MAC_SOURCE}
        "${CMAKE_CURRENT_SOURCE_DIR}/src/app/Main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/system/UserDataPaths.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/editor_restart_helper/Main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/benchmarks/Main.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/asset_check/Main.cpp"
    )
    list(REMOVE_DUPLICATES GAME_WARNING_SOURCES)
    foreach(source IN LISTS GAME_WARNING_SOURCES)
        # 公開フックの引数名を残し、第三者ライブラリの実装には警告設定を追加しない。
        if (MSVC)
            set_property(SOURCE ${source} APPEND PROPERTY COMPILE_OPTIONS /W4 /wd4100)
        else()
            set_property(SOURCE ${source} APPEND PROPERTY COMPILE_OPTIONS
                -Wall -Wextra -Wpedantic -Wno-unused-parameter)
        endif()
    endforeach()
endif()

if (GAME_ENABLE_COVERAGE)
    if (MSVC OR NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "GAME_ENABLE_COVERAGE requires Clang with a GNU-style command line")
    endif()
    target_compile_options(game_runtime PUBLIC -fprofile-instr-generate -fcoverage-mapping)
    target_link_options(game_runtime INTERFACE -fprofile-instr-generate)
    set(GAME_COVERAGE_OBJECTS "$<TARGET_FILE:game>\n")
    foreach(target game_tests editor_restart_helper restart_launch_probe game_asset_check)
        if (TARGET ${target})
            target_compile_options(${target} PRIVATE -fprofile-instr-generate -fcoverage-mapping)
            target_link_options(${target} PRIVATE -fprofile-instr-generate)
            string(APPEND GAME_COVERAGE_OBJECTS "$<TARGET_FILE:${target}>\n")
        endif()
    endforeach()
    foreach(target game_runtime_tests game_startup_probe)
        if (TARGET ${target})
            string(APPEND GAME_COVERAGE_OBJECTS "$<TARGET_FILE:${target}>\n")
        endif()
    endforeach()
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/coverage")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/coverage/objects-$<CONFIG>.txt"
        CONTENT "${GAME_COVERAGE_OBJECTS}")
    get_property(GAME_REGISTERED_TESTS DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS GAME_REGISTERED_TESTS)
        set_property(TEST ${test} APPEND PROPERTY ENVIRONMENT
            "LLVM_PROFILE_FILE=${CMAKE_CURRENT_BINARY_DIR}/coverage/%p-%m.profraw")
    endforeach()
endif()

if (GAME_ENABLE_RUNTIME_SANITIZERS)
    # RuntimeにリンクしないCLIも、同じ依存ライブラリとエラー経路を計装する。
    foreach(target editor_restart_helper restart_launch_probe game_asset_check)
        if (TARGET ${target})
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined
                -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endforeach()
endif()
