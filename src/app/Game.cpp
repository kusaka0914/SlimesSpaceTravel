#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// GLEWはapp/Game.hが取り込むGLFW/OpenGLヘッダより先に読み込む。
#include <GL/glew.h>

#include "app/Game.h"
#include "world/Stage.h"
#include <GLFW/glfw3.h>

#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "app/GameInputController.h"
#include "app/GameSessionController.h"
#include "editor/session/DebugEditorSessionController.h"
#include "state/GameProgressState.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
#include "system/CameraSystem.h"
#include "system/EnemyJewelDropSystem.h"
#include "system/GameProgressController.h"
#include "system/GamepadRumbleService.h"
#include "system/InputSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/ParticleSystem.h"
#include "system/PauseMenuController.h"
#include "system/PhysicsSystem.h"
#include "system/PlayerConfigurationController.h"
#include "system/SceneSystem.h"
#include "system/StageFlowController.h"
#include "system/UGCModeController.h"
#include "system/UILoadSystem.h"
#include "system/UserDataPaths.h"
#include "system/sequence/SequenceSystem.h"
#include "world/GameWorld.h"

#include "app/GameFrameRenderer.h"
#include "editor/core/UIDebugEditorBridge.h"
#include "gameplay/ugc/UGCPreviewController.h"
#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"
#include "imgui.h"

#include "actor/MathUtils.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
#include <utility>

namespace {

std::string BuildStageIntroCinematicId(int stageNum)
{
    if (stageNum <= 0) {
        return {};
    }

    return "enter_stage" + std::to_string(stageNum);
}

} // namespace

Game::Game() = default;

Game::~Game()
{
    Shutdown();
}

bool Game::Initialize(bool areDebugToolsEnabled, bool shouldStartInDebugStage, const std::string& editorSessionPath,
                      const std::string& editorRestartErrorLogPath)
{
    mIsDebugMode = areDebugToolsEnabled;

    const bool shouldUseFullscreen = !shouldStartInDebugStage;
    if (!mWindow.Initialize(shouldUseFullscreen)) {
        return false;
    }

    if (!IsTgsBuild()) {
        std::string userDataErrorMessage;
        if (!UserDataPaths::PrepareFromPackagedAssets("../assets/data", userDataErrorMessage)) {
            std::cerr << userDataErrorMessage << '\n';
        }
    }

    if (!CreateGameSystems()) {
        Shutdown();
        return false;
    }
    InitializeGameController();

    constexpr int stageCount = 7;
    CreateStages(stageCount);

    if (shouldStartInDebugStage) {
        mWorld->ChangeStage(1);
        mStageFlowController->SetCurrentStageYamlPath("../assets/data/stage/test.yaml");
        mSceneSystem->StartPlayingScene();
    }

    if (!ReloadCurrentStage()) {
        Shutdown();
        return false;
    }
    RestoreDebugEditorSessionAtStartup(editorSessionPath, editorRestartErrorLogPath);

    mLastTime = glfwGetTime();
    glEnable(GL_DEPTH_TEST);
    mWindow.SetCursorVisible(mIsDebugEditorShowing);
    mWindow.SetTextInputAllowed(IsEditorKeyboardInputCaptured());

    return true;
}

void Game::InitializeGameController()
{
    mGamepadRumbleService->Initialize();
}

bool Game::CreateGameSystems()
{
    mUGCModeController = std::make_unique<UGCModeController>(*this);
    mUGCPreviewController = std::make_unique<UGCPreviewController>(this);
    mWorld = std::make_unique<GameWorld>();
    mPauseMenuController = std::make_unique<PauseMenuController>();
    mStageFlowController = std::make_unique<StageFlowController>();
    mGamepadRumbleService = std::make_unique<GamepadRumbleService>();
    mEnemyJewelDropSystem = std::make_unique<EnemyJewelDropSystem>(this);

    mAudioSystem = std::make_unique<AudioSystem>(this);
    mUIRenderer = std::make_unique<UIRenderer>(this);
    if (!mUIRenderer->IsInitialized()) {
        std::cerr << "Failed to initialize the UI renderer.\n";
        return false;
    }

    // EditorはUIの描画機能を借用し、ImGuiの寿命と編集操作を所有する。
    mEditor = std::make_unique<UIDebugEditorBridge>(*this, *mUIRenderer);
    if (!mEditor->IsInitialized()) {
        std::cerr << "Failed to initialize the editor UI.\n";
        return false;
    }
    mDebugEditorSessionController =
        std::make_unique<DebugEditorSessionController>(mIsDebugMode, *GetWindow(), *mEditor);
    mSceneSystem = std::make_unique<SceneSystem>(this, *mUIRenderer->GetUILoadSystem());
    mMathUtils = std::make_unique<MathUtils>();
    mInputSystem = std::make_unique<InputSystem>();
    mCameraSystem = std::make_unique<CameraSystem>(CameraSystemDependencies{
        .world = *mWorld,
        .input = *mInputSystem,
        .scene = *mSceneSystem,
        .audio = mAudioSystem.get(),
        .physicsProvider = [this]() { return mPhysicsSystem.get(); },
        .readControlState =
            [this]() {
                return CameraControlState{
                    .mainPlayer = GetMainPlayer(),
                    .controlledPlayer = GetControlledPlayer(),
                    .controlledPlayerIndex = GetControlledPlayerIndex(),
                    .isPlayer2Joined = GetIsPlayer2Joined(),
                    .isFreeCameraMode = GetIsFreeCameraMode(),
                    .isUGCMode = GetIsUGCMode(),
                    .isDebugEditorShowing = GetIsDebugEditorShowing(),
                    .isEditorKeyboardInputCaptured = IsEditorKeyboardInputCaptured(),
                    .hasPrimaryController = IsGameControllerConnected(),
                    .hasSecondController = HasGameControllerForPlayer(2),
                };
            },
        .findPlacedActor = [this](const std::string& sequenceName, int yamlIndex) -> Actor* {
            Stage* stage = mWorld->GetCurrentStage();
            return mActorLoadSystem && stage
                       ? mActorLoadSystem->GetActorLocator().FindPlacedActor(*stage, sequenceName, yamlIndex)
                       : nullptr;
        },
    });
    mMeshLoadSystem = std::make_unique<MeshLoadSystem>();
    mUGCPreviewController->SetMeshLoadSystem(*mMeshLoadSystem);
    mActorLoadSystem = std::make_unique<ActorLoadSystem>(this, *mWorld);
    mPhysicsSystem = std::make_unique<PhysicsSystem>(this);
    mProgressController = std::make_unique<GameProgressController>(
        *mWorld, *mPhysicsSystem, mStageFlowController->GetCurrentStageYamlPath(), !IsTgsBuild());
    mProgressController->Load();
    if (mProgressController->HasSelectedPlayerControlStyle()) {
        mPlayerControlStyle = mProgressController->GetSelectedPlayerControlStyle();
    }
    mSequenceSystem = std::make_unique<SequenceSystem>(this);

    // 参照する各Systemを生成してから、Player構成を管理するControllerへ依存先を渡す。
    mPlayerConfigurationController = std::make_unique<PlayerConfigurationController>(PlayerConfigurationDependencies{
        .world = *mWorld,
        .playerLoader = mActorLoadSystem->GetPlayerLoader(),
        .sceneSystem = *mSceneSystem,
        .cameraSystem = *mCameraSystem,
        .sequenceSystem = *mSequenceSystem,
        .progressController = *mProgressController,
        .gamepadService = *mGamepadRumbleService,
        .pauseMenuController = *mPauseMenuController,
        .physicsSystem = *mPhysicsSystem,
        .allowsKeyboardOnlyTwoPlayer = mIsDebugMode,
    });

    mSessionController = std::make_unique<GameSessionController>(
        GameSessionDependencies{
            .world = *mWorld,
            .sceneSystem = *mSceneSystem,
            .sequenceSystem = *mSequenceSystem,
            .playerConfigurationController = *mPlayerConfigurationController,
            .progressController = *mProgressController,
            .pauseMenuController = *mPauseMenuController,
            .inputSystem = *mInputSystem,
            .suppressOneShotInput =
                [this]() {
                    if (mGameInputController) {
                        mGameInputController->SuppressOneShotInputUntilReleased();
                    }
                },
            .audioSystem = *mAudioSystem,
            .ugcModeController = *mUGCModeController,
            .playerControlStyle = mPlayerControlStyle,
        },
        IsTgsBuild());

    mGameInputController =
        std::make_unique<GameInputController>(*this, *mInputSystem, *mEditor, *mSessionController, *mUGCModeController);

    mParticleSystem = std::make_unique<ParticleSystem>();
    mParticleSystem->LoadDefinitions("../assets/data/effects/particles.yaml");
    mRenderer3D = std::make_unique<Renderer3D>(Renderer3DDependencies{
        .window = *GetWindow(),
        .math = *mMathUtils,
        .models = *mMeshLoadSystem,
        .physics = *mPhysicsSystem,
        .uiDefinitions = *mUIRenderer->GetUILoadSystem(),
        .cameras = *mCameraSystem,
        .performance = mFramePerformanceTracker,
        .readScene = [this]() { return CreateSceneRenderState(); },
    });
    if (!mRenderer3D->IsInitialized()) {
        std::cerr << "Failed to initialize the 3D renderer.\n";
        return false;
    }
    mFrameRenderer =
        std::make_unique<GameFrameRenderer>(*GetWindow(), *mRenderer3D, *mUIRenderer, *mEditor, *mCameraSystem,
                                            *mUGCPreviewController, mFramePerformanceTracker);
    return true;
}

void Game::CreateStages(int stageCount)
{
    mWorld->CreateStages(stageCount);
}

bool Game::ReloadCurrentStage(StagePhysicsReloadMode physicsReloadMode)
{
    const auto previousPlayerConfiguration = mPlayerConfigurationController->CaptureStageReloadState();
    if (mSequenceSystem) {
        mSequenceSystem->Stop(true);
    }

    if (mCameraSystem) {
        mCameraSystem->ResetForStageChange();
    }

    if (mParticleSystem) {
        mParticleSystem->Clear();
    }

    // Actorの破棄に先立って、演出・UIが保持する旧ステージへの参照を解除する。
    if (mSceneSystem) {
        mSceneSystem->ClearActorReferencesForStageReload();
    }

    if (mEditor) {
        mEditor->ClearStageSelectionForStageReload();
    }

    if (physicsReloadMode == StagePhysicsReloadMode::SkipRebuild && mPhysicsSystem) {
        mPhysicsSystem->ClearForEditorStageRebuild();
    }

    const bool wasStageReloaded =
        mStageFlowController->ReloadCurrentStage(*mActorLoadSystem, *mPhysicsSystem, *mAudioSystem, physicsReloadMode);
    if (!wasStageReloaded) {
        mPlayerConfigurationController->RestoreStageReloadState(previousPlayerConfiguration);
        if (physicsReloadMode == StagePhysicsReloadMode::SkipRebuild && mPhysicsSystem) {
            mPhysicsSystem->Initialize();
        }
        return false;
    }

    if (mEditor) {
        mEditor->NotifyStageReloaded();
    }

    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->ClearRuntimeDrops();
    }

    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->SynchronizeAfterStageReload();
    }

    if (mCameraSystem) {
        mCameraSystem->SnapBehindControlledPlayer();
    }
    return true;
}

void Game::ReloadUIData()
{
    if (!mUIRenderer) {
        return;
    }

    mUIRenderer->GetUILoadSystem()->Initialize();
}

SceneRenderState Game::CreateSceneRenderState() const
{
    SceneRenderState scene{
        .stage = mWorld->GetCurrentStage(),
        .stageNumber = mWorld->GetCurrentStageNum(),
        .players = mWorld->GetPlayers(),
        .runtimeJewels = GetRuntimeJewelItems(),
        .particles = mParticleSystem->GetParticles(),
        .controlledPlayer = GetControlledPlayer(),
        .isTitle = mSceneSystem->IsTitle(),
        .isBattleStyleSelection = mSceneSystem->IsBattleStyleSelection(),
        .hidesPlayersForCinematic = mSequenceSystem->IsCinematicChainPlaying(),
        .isUGCMode = GetIsUGCMode(),
        .isDebugEditorShowing = mIsDebugEditorShowing,
        .isDebugMode = mIsDebugMode,
        .isGameUIHidden = mIsGameUIHidden,
        .isPlayer2Joined = GetIsPlayer2Joined(),
        .projection = {GetIsUGCMode() && GetIsUGCOrthographicView(), GetUGCOrthographicHalfHeight()},
    };
    if (!scene.isGameUIHidden && GetIsPlayerSplit() && !scene.isPlayer2Joined) {
        scene.splitGuard = {scene.controlledPlayer, GetPlayerSplitGuardCount(), GetMaximumPlayerSplitGuardCount()};
    }
    if (!TryResolvePlayerMergeGuide(scene.mergeGuideTarget, scene.mergeGuideRadiusWorldUnits)) {
        scene.mergeGuideTarget = nullptr;
        scene.mergeGuideRadiusWorldUnits = 0.0f;
    }
    scene.ugcEditLayer = mUGCPreviewController->GetEditLayer();
    scene.ugcGridSize = mUGCPreviewController->GetGridSize();
    scene.placementPosition = mUGCPreviewController->GetPlatformPlacementPosition();
    scene.placementModel = mUGCPreviewController->GetPlacementModel();
    scene.placementModelPositions = mUGCPreviewController->GetPlacementModelPositions();
    scene.movingPathStart = mUGCPreviewController->GetMovingPlatformPathStart();
    scene.movingPathDestination = mUGCPreviewController->GetMovingPlatformPathDestination();
    return scene;
}

void Game::RunLoop()
{
    constexpr int maximumFramesPerSecond = 60;
    const auto minimumFrameDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(1.0 / static_cast<double>(maximumFramesPerSecond)));

    while (!glfwWindowShouldClose(GetWindow())) {
        const auto frameStartTime = std::chrono::steady_clock::now();
        mFramePerformanceTracker.BeginFrame();
        mFrameRenderer->PollGpuPerformanceMeasurements();
        glfwPollEvents();
        ProcessInput();

        const auto gameUpdateStartTime = std::chrono::steady_clock::now();
        UpdateGame();
        const auto gameUpdateEndTime = std::chrono::steady_clock::now();
        mFramePerformanceTracker.RecordGameUpdateDuration(
            std::chrono::duration<float, std::milli>(gameUpdateEndTime - gameUpdateStartTime).count());
        mWindow.SetCursorVisible(mIsDebugEditorShowing);

        const GameFrameRenderState renderState{
            .isDebugEditorShowing = mIsDebugEditorShowing,
            .isUGCModeActive = GetIsUGCMode(),
            .isUGCWorkBrowserShowing = GetIsUGCWorkBrowserShowing(),
            .isUGCOrthographicView = GetIsUGCOrthographicView(),
            .deltaTimeSeconds = mLastDeltaTime,
        };
        mFrameRenderer->Render(renderState);
        mWindow.SetTextInputAllowed(IsEditorKeyboardInputCaptured());

        mFramePerformanceTracker.RecordTotalDuration(
            std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - frameStartTime).count());

        // ドライバー側でVSyncが無効化されても過剰に描画し続けないよう、ソフトウェア側でも60FPSに制限する。
        std::this_thread::sleep_until(frameStartTime + minimumFrameDuration);
    }
}

void Game::Shutdown()
{
    SavePersistentDebugEditorSession();

    if (mGamepadRumbleService) {
        mGamepadRumbleService->Shutdown();
    }

    if (mAudioSystem) {
        mAudioSystem->Shutdown();
    }

    // OpenGLの描画資源は、ウィンドウとコンテキストが有効な間に解放する。
    if (mFrameRenderer) {
        mFrameRenderer->Shutdown();
        mFrameRenderer.reset();
    }
    if (mRenderer3D) {
        mRenderer3D->Shutdown();
        mRenderer3D.reset();
    }
    mGameInputController.reset();
    mDebugEditorSessionController.reset();
    mEditor.reset();

    if (mUIRenderer) {
        mUIRenderer->Shutdown();
        mUIRenderer.reset();
    }

    if (mMeshLoadSystem) {
        mMeshLoadSystem->Shutdown();
    }

    SDL_Quit();

    mWindow.Shutdown();
}

void Game::RestoreDebugEditorSessionAtStartup(const std::string& editorSessionPath,
                                              const std::string& editorRestartErrorLogPath)
{
    if (!mDebugEditorSessionController) {
        return;
    }

    const DebugEditorSessionRestoreOutcome restoreOutcome =
        mDebugEditorSessionController->RestoreAtStartup(editorSessionPath, editorRestartErrorLogPath);
    if (restoreOutcome.shouldShowEditor) {
        mIsDebugEditorShowing = true;
    }
}

void Game::SavePersistentDebugEditorSession()
{
    if (mDebugEditorSessionController) {
        mDebugEditorSessionController->SavePersistentSession();
    }
}

void Game::ProcessInput()
{
    SDL_PumpEvents();
    SDL_GameControllerUpdate();

    if (mInputSystem) {
        mInputSystem->CaptureFrameInput(*GetWindow(), {mGamepadRumbleService->GetControllerForPlayer(1),
                                                       mGamepadRumbleService->GetControllerForPlayer(2)});
        mSessionController->ProcessInput();
        if (ShouldBlockInputForTgsPresentation()) {
            return;
        }
        mGameInputController->ProcessGameInput();
    }

    if (mPauseMenuController->IsOpen()) {
        return;
    }

    ProcessActorsInput();
    mCameraSystem->ProcessInput();
}

void Game::MovePauseMenuSelection(int delta)
{
    mPauseMenuController->MoveSelection(*this, delta);
}

void Game::TogglePauseMenu()
{
    if (!mPauseMenuController->IsOpen() && !CanOpenPauseMenu()) {
        return;
    }
    mPauseMenuController->Toggle();
}

void Game::ClosePauseMenu()
{
    mPauseMenuController->Close();
}

bool Game::CanOpenPauseMenu() const
{
    return mSceneSystem && mSceneSystem->IsPlaying() && mCameraSystem && mCameraSystem->AllowsPlayerInput() &&
           (!mSequenceSystem || !mSequenceSystem->LocksPlayerControl());
}

void Game::ExecutePauseMenuItem()
{
    mPauseMenuController->ExecuteSelectedItem(*this);
}

void Game::ReturnToBase()
{
    mStageFlowController->ReturnToBase(*mPauseMenuController, *mWorld, *mSceneSystem);
}

void Game::OpenFeedbackForm()
{
    const char* url =
        "https://docs.google.com/forms/d/e/1FAIpQLSdv81tlscrZ9gVi38bVqnHZ3aCfo0jD-iLgBGjDh9TYqNj8Qg/viewform";

    if (SDL_OpenURL(url) != 0) {
        std::cerr << "Failed to open URL: " << SDL_GetError() << std::endl;
    }
}

void Game::TryCreatePlayer2()
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->JoinSecondPlayer();
    }
}

bool Game::IsReviewBuild() const
{
#ifdef GAME_REVIEW_BUILD
    return true;
#else
    return false;
#endif
}

void Game::ReturnToSinglePlayer()
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->ReturnToSinglePlayer();
    }
}

bool Game::CanStartTwoPlayerFromPauseMenu() const
{
    return mPlayerConfigurationController && mPlayerConfigurationController->CanStartTwoPlayerFromPauseMenu();
}

bool Game::CanReturnToBaseFromPauseMenu() const
{
    return IsStageCleared(1) && !IsInBase();
}

void Game::ToggleDebugEditor()
{
    if (GetIsUGCMode()) {
        mUGCModeController->ToggleDebugPanel();
        return;
    }

    mIsDebugEditorShowing = !mIsDebugEditorShowing;
    if (mPhysicsSystem) {
        mPhysicsSystem->SyncKinematicBodies();
    }
}

bool Game::IsEditorKeyboardInputCaptured() const
{
    if (!mIsDebugEditorShowing || !ImGui::GetCurrentContext()) {
        return false;
    }

    return ImGui::GetIO().WantTextInput;
}

void Game::ToggleFreeCameraMode()
{
    SetFreeCameraMode(!mIsFreeCameraMode);
}

void Game::SetFreeCameraMode(bool isEnabled)
{
    mIsFreeCameraMode = isEnabled;
}

bool Game::RequestEditorBuildAndRestart(std::string& outErrorMessage)
{
    if (!mDebugEditorSessionController) {
        outErrorMessage = "The editor build restart service is not available.";
        return false;
    }
    return mDebugEditorSessionController->RequestBuildAndRestart(outErrorMessage);
}

void Game::ProcessActorsInput()
{
    const bool isWaitingForTutorialPlayerJump = mSceneSystem->IsWaitingForTutorialPlayerJump();
    const bool allowsPlayerControl =
        (mSceneSystem->IsPlaying() || isWaitingForTutorialPlayerJump) && !IsEditorKeyboardInputCaptured() &&
        mCameraSystem->AllowsPlayerInput() && (!mSequenceSystem || !mSequenceSystem->LocksPlayerControl()) &&
        (!mPlayerConfigurationController || !mPlayerConfigurationController->IsSplitMergeTransitionActive());

    const std::vector<Player*>& players = GetPlayers();
    for (Player* player : players) {
        if (player) {
            player->SetControlLocked(!allowsPlayerControl);
        }
    }

    if (!allowsPlayerControl) {
        return;
    }

    if (isWaitingForTutorialPlayerJump) {
        mWorld->ProcessPlayerInput(GetControlledPlayer());
        return;
    }

    mWorld->ProcessActorsInput();
}

const FramePerformanceMetrics& Game::GetFramePerformanceMetrics() const
{
    return mFramePerformanceTracker.GetMetrics();
}

void Game::UpdateGame()
{
    CheckGameControllerConnected();

    const double currentTime = glfwGetTime();
    const float elapsedSeconds = std::max(0.0f, static_cast<float>(currentTime - mLastTime));
    const float deltaTime = std::min(0.04f, elapsedSeconds);
    mLastTime = currentTime;
    mLastDeltaTime = deltaTime;

    // 体験時間には実経過時間を使い、物理・演出の更新には大きな時間差を制限したdeltaTimeを使う。
    mSessionController->Update(elapsedSeconds);

    ProcessPendingUGCClearCompletion();

    if (mHitStopTimer >= 0.0f) {
        mHitStopTimer -= deltaTime;
        if (mCameraSystem) {
            // ヒットストップ中も画面シェイクだけは時間を進める。
            // 通常の追従更新は止めたままなので、衝撃がカメラ基準位置を乱さない。
            mCameraSystem->UpdateShakeEffects(deltaTime);
        }
        return;
    }

    if (mPauseMenuController->IsOpen()) {
        return;
    }

    mSceneSystem->Update(deltaTime);
    UpdatePendingSoloSplitControlSwitch(deltaTime);

    if (mIsFreeCameraMode) {
        if (mSequenceSystem) {
            mSequenceSystem->Update(deltaTime);
        }
        mCameraSystem->Update(deltaTime);
        return;
    }

    bool wasCameraUpdated = false;
    const bool shouldUpdateEntireWorld = (mSceneSystem->CanUpdateWorld() || mSceneSystem->IsStageClear()) &&
                                         !GetIsUGCClearResultShowing() && !ShouldBlockInputForTgsPresentation();
    const bool shouldUpdateTutorialPlayer = mSceneSystem->IsWaitingForTutorialPlayerJump();

    // 操作を待つチュートリアルではワールド全体を止め、操作中のPlayerだけを更新する。
    if (shouldUpdateEntireWorld) {
        UpdateActors(deltaTime);

        if (mParticleSystem) {
            mParticleSystem->Update(deltaTime);
        }
    } else if (shouldUpdateTutorialPlayer) {
        mWorld->UpdatePlayer(GetControlledPlayer(), deltaTime);
    }

    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->UpdateSplitMergeTransition(deltaTime);
    }

    if (mSequenceSystem) {
        mSequenceSystem->Update(deltaTime);
    }

    if (shouldUpdateEntireWorld || shouldUpdateTutorialPlayer) {
        mCameraSystem->Update(deltaTime);
        wasCameraUpdated = true;
    }

    if (!wasCameraUpdated && (mCameraSystem->IsCinematicPlaying() || mSceneSystem->IsTalking())) {
        mCameraSystem->Update(deltaTime);
    }
}

void Game::UpdateActors(float deltaTime)
{
    // Actor更新中の生成で走査中のコンテナを変えないよう、予約したドロップは更新後に生成する。
    mWorld->UpdateActors(deltaTime);
    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->SpawnPendingDrops();
    }
}

bool Game::IsTgsBuild() const
{
#ifdef GAME_TGS_BUILD
    return true;
#else
    return false;
#endif
}

bool Game::IsTgsThankYouScreenVisible() const
{
    return mSessionController && mSessionController->IsTgsThankYouScreenVisible();
}

bool Game::IsTgsRemainingTimeNoticeVisible() const
{
    return mSessionController && mSessionController->IsTgsRemainingTimeNoticeVisible();
}

bool Game::ShouldBlockInputForTgsPresentation() const
{
    return mSessionController && mSessionController->ShouldBlockInputForTgsPresentation();
}

void Game::SetUGCPreviewRenderSize(int width, int height)
{
    mUGCPreviewController->SetRenderSize(width, height);
}

void Game::AdjustUGCPreviewYaw(float yawDeltaRadians)
{
    mUGCPreviewController->AdjustYaw(yawDeltaRadians);
}

float Game::GetUGCPreviewYawRadians() const
{
    return mUGCPreviewController->GetYaw();
}

void Game::ToggleUGCPreviewVerticalView()
{
    mUGCPreviewController->ToggleVerticalView();
}

bool Game::GetIsUGCPreviewViewedFromBelow() const
{
    return mUGCPreviewController->IsViewedFromBelow();
}

float Game::GetUGCPreviewFocusY() const
{
    return mUGCPreviewController->GetFocusY();
}

void Game::SetUGCPreviewEditLayer(int gridLayer)
{
    mUGCPreviewController->SetEditLayer(gridLayer);
}

int Game::GetUGCPreviewEditLayer() const
{
    return mUGCPreviewController->GetEditLayer();
}

void Game::SetUGCPlatformPlacementPreview(const std::optional<glm::vec3>& position)
{
    mUGCPreviewController->SetPlatformPlacementPosition(position);
}

void Game::SetUGCMovingPlatformPathPreview(const std::optional<glm::vec3>& startPosition,
                                           const std::optional<glm::vec3>& destinationPosition)
{
    mUGCPreviewController->SetMovingPlatformPath(startPosition, destinationPosition);
}

void Game::SetUGCOrthographicHalfHeight(float halfHeight)
{
    mUGCPreviewController->SetOrthographicHalfHeight(halfHeight);
}

float Game::GetUGCOrthographicHalfHeight() const
{
    return mUGCPreviewController->GetOrthographicHalfHeight();
}

float Game::GetUGCGridSize() const
{
    return mUGCPreviewController->GetGridSize();
}

unsigned int Game::GetUGCPreviewTexture() const
{
    return mFrameRenderer ? mFrameRenderer->GetUGCPreviewTexture() : 0;
}

bool Game::GetIsUGCEditorTutorialActive() const
{
    return mUGCModeController && mUGCModeController->IsEditorTutorialActive();
}

bool Game::GetIsUGCWorkBrowserShowing() const
{
    return mUGCModeController && mUGCModeController->IsWorkBrowserShowing();
}

bool Game::GetIsUGCMode() const
{
    return mUGCModeController && mUGCModeController->IsModeActive();
}

bool Game::GetIsUGCPlaytestActive() const
{
    return mUGCModeController && mUGCModeController->IsPlaytestActive();
}

bool Game::GetIsUGCClearVerificationActive() const
{
    return mUGCModeController && mUGCModeController->IsVerificationActive();
}

bool Game::GetIsUGCDebugEditorShowing() const
{
    return mUGCModeController && mUGCModeController->IsDebugPanelShowing();
}

bool Game::GetIsUGCOrthographicView() const
{
    return mUGCModeController && mUGCModeController->IsOrthographicView();
}

void Game::SetIsUGCOrthographicView(bool isOrthographic)
{
    if (mUGCModeController) {
        mUGCModeController->SetOrthographicView(isOrthographic);
    }
}

void Game::AddActor(std::unique_ptr<Actor> actor)
{
    mWorld->AddActor(std::move(actor));
}

void Game::SetUGCPlacementModelPreview(const std::optional<glm::vec3>& position, const std::string& modelPath,
                                       const glm::vec3& scale, const std::string& textureOverridePath)
{
    mUGCPreviewController->SetPlacementModel(position, modelPath, scale, textureOverridePath);
}

void Game::SetUGCPlacementModelPreviewPositions(const std::vector<glm::vec3>& positions, const std::string& modelPath,
                                                const glm::vec3& scale, const std::string& textureOverridePath)
{
    mUGCPreviewController->SetPlacementModelPositions(positions, modelPath, scale, textureOverridePath);
}

void Game::RequestEnemyJewelDrop(const Enemy& defeatedEnemy)
{
    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->RequestDrop(defeatedEnemy);
    }
}

const std::vector<JewelItem*>& Game::GetRuntimeJewelItems() const
{
    static const std::vector<JewelItem*> emptyItems;
    return mEnemyJewelDropSystem ? mEnemyJewelDropSystem->GetRuntimeItems() : emptyItems;
}

void Game::AddPlayer(Player* player)
{
    if (!player) {
        return;
    }

    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->ConfigureAddedPlayer(*player);
    }
    mWorld->AddPlayer(player);
}

void Game::RemoveAllPlayer()
{
    mWorld->RemoveAllPlayers();
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->Reset();
    }
}

bool Game::TogglePlayerSplit()
{
    return mPlayerConfigurationController && mPlayerConfigurationController->ToggleSplit();
}

void Game::SetPlayerSplitMergeButtonHeld(bool isHeld)
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->SetSplitMergeButtonHeld(isHeld);
    }
}

bool Game::TryResolvePlayerMergeGuide(const Player*& targetPlayer, float& radiusWorldUnits) const
{
    if (!mPlayerConfigurationController) {
        targetPlayer = nullptr;
        radiusWorldUnits = 0.0f;
        return false;
    }
    return mPlayerConfigurationController->TryResolveMergeGuide(targetPlayer, radiusWorldUnits);
}

bool Game::TryConsumePlayerSplitGuard(const Player& damagedPlayer)
{
    return mPlayerConfigurationController && mPlayerConfigurationController->TryConsumeSplitGuard(damagedPlayer);
}

int Game::GetPlayerSplitGuardCount() const
{
    return mPlayerConfigurationController ? mPlayerConfigurationController->GetSplitGuardCount() : 0;
}

int Game::GetMaximumPlayerSplitGuardCount() const
{
    return mPlayerConfigurationController ? mPlayerConfigurationController->GetMaximumSplitGuardCount() : 0;
}

bool Game::CanTogglePlayerSplit() const
{
    return mPlayerConfigurationController && mPlayerConfigurationController->CanToggleSplit();
}

bool Game::SwitchControlledPlayer()
{
    return mPlayerConfigurationController && mPlayerConfigurationController->SwitchControlledPlayer();
}

bool Game::CanSwitchControlledPlayer() const
{
    return mPlayerConfigurationController && mPlayerConfigurationController->CanSwitchControlledPlayer();
}

void Game::RequestSoloSplitControlSwitchAfterBoarding()
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->RequestControlSwitchAfterBoarding();
    }
}

void Game::UpdatePendingSoloSplitControlSwitch(float deltaTime)
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->UpdatePendingControlSwitch(deltaTime);
    }
}

void Game::ChangeStage(int stageNum)
{
    mStageFlowController->ChangeStage(*mWorld, stageNum);
}

bool Game::HasStageIntroCinematic(int stageNum) const
{
    if (!mCameraSystem) {
        return false;
    }

    const std::string cinematicId = BuildStageIntroCinematicId(stageNum);
    if (cinematicId.empty()) {
        return false;
    }

    return mCameraSystem->GetCinematicLibrary().Find(cinematicId) != nullptr;
}

bool Game::StartStageIntroCinematic(int stageNum)
{
    if (!mSequenceSystem || !mAudioSystem || !HasStageIntroCinematic(stageNum)) {
        return false;
    }

    const std::string cinematicId = BuildStageIntroCinematicId(stageNum);
    if (!mSequenceSystem->PlayCinematicChain({cinematicId})) {
        return false;
    }

    mAudioSystem->PlayBGMOnce("enter_stage_bgm");
    return true;
}

bool Game::DebugChangeStage(int stageNum, const std::string& yamlPath)
{
    if (!LoadDebugStage(stageNum, yamlPath)) {
        return false;
    }

    const bool isBaseStageYaml = yamlPath.ends_with("/stage0.yaml") || yamlPath.ends_with("\\stage0.yaml");
    if (isBaseStageYaml && mSequenceSystem) {
        if (!HasSeenBaseIntro() &&
            mSequenceSystem->PlayCinematicChainThenSequence({"base_sequence"}, "base_arrival_template")) {
            MarkBaseIntroSeen();
            if (Player* player = GetMainPlayer()) {
                player->SetIsActive(false);
            }
        } else {
            mSequenceSystem->Play("base_arrival_template");
        }
    } else if (HasStageIntroCinematic(stageNum)) {
        mAudioSystem->BeginStageMusicDeferral();
        if (!StartStageIntroCinematic(stageNum)) {
            mAudioSystem->ResumeDeferredStageMusic();
        }
    }

    return true;
}

bool Game::DebugEnterTitle()
{
    if (!PrepareInitialSceneForDebug()) {
        return false;
    }

    mSceneSystem->DebugEnterTitle();
    mSessionController->ResetTitleMenuSelection();
    mAudioSystem->TryChangeBGM();
    return true;
}

bool Game::DebugEnterOpening()
{
    if (!PrepareInitialSceneForDebug()) {
        return false;
    }

    mSceneSystem->DebugEnterOpening();
    mAudioSystem->TryChangeBGM();
    return true;
}

bool Game::DebugEnterEnding()
{
    if (!PrepareInitialSceneForDebug()) {
        return false;
    }

    mSceneSystem->DebugEnterEnding();
    mAudioSystem->TryChangeBGM();
    return true;
}

bool Game::DebugStartCredits()
{
    if (!PrepareInitialSceneForDebug()) {
        return false;
    }

    mSceneSystem->DebugStartCredits();
    mAudioSystem->TryChangeBGM();
    return true;
}

bool Game::IsTitleScene() const
{
    return mSceneSystem && mSceneSystem->IsTitle();
}

void Game::SetDebugCameraPose(const CameraPose& pose)
{
    if (mCameraSystem) {
        mCameraSystem->SetDebugCameraPose(pose);
    }
}

bool Game::RequestSceneFadeAction(const std::function<void()>& fadeAction)
{
    return mSceneSystem && mSceneSystem->RequestFadeAction(fadeAction);
}

void Game::NotifyUGCTutorialReturnedFromPlaytest()
{
    if (mEditor) {
        mEditor->NotifyTutorialReturnedFromPlaytest();
    }
}

void Game::CompleteUGCVerification(const std::string& workFileName)
{
    if (mEditor) {
        mEditor->CompleteUGCVerification(workFileName);
    }
}

bool Game::HasProgressFlag(const std::string& progressId) const
{
    return mProgressController && mProgressController->HasProgressFlag(progressId);
}

void Game::MarkProgressFlag(const std::string& progressId)
{
    if (mProgressController) {
        mProgressController->MarkProgressFlag(progressId);
    }
}

void Game::EnterTitleAtFadeMidpoint()
{
    if (mSceneSystem) {
        mSceneSystem->EnterTitleAtFadeMidpoint();
    }
}

void Game::TryChangeBGM()
{
    if (mAudioSystem) {
        mAudioSystem->TryChangeBGM();
    }
}

bool Game::StartUGCMode()
{
    return mUGCModeController->StartMode();
}

bool Game::StartUGCEditorTutorial()
{
    return mUGCModeController->StartEditorTutorial();
}

bool Game::FinishUGCEditorTutorial(bool wasCompleted)
{
    return mUGCModeController->FinishEditorTutorial(wasCompleted);
}

int Game::GetTitleMenuSelection() const
{
    return mSessionController ? mSessionController->GetTitleMenuSelection() : 0;
}

void Game::CloseUGCWorkBrowser()
{
    mUGCModeController->CloseWorkBrowser();
}

void Game::StartUGCPlaytest()
{
    mUGCModeController->StartPlaytest();
}

void Game::StartUGCSavedWorkPlaytest()
{
    mUGCModeController->StartSavedWorkPlaytest();
}

void Game::StartUGCClearVerification(const std::string& workFileName)
{
    mUGCModeController->StartClearVerification(workFileName);
}

void Game::ExitUGCMode()
{
    mUGCModeController->ExitMode();
}

bool Game::GetIsUGCClearResultShowing() const
{
    return mUGCModeController && mUGCModeController->IsClearResultShowing();
}

int Game::GetUGCClearResultSelection() const
{
    return mUGCModeController ? mUGCModeController->GetClearResultSelection() : 0;
}

bool Game::PrepareInitialSceneForDebug()
{
    if (!mIsDebugMode) {
        return false;
    }

    constexpr int initialStageNumber = 0;
    constexpr const char* initialStageYamlPath = "../assets/data/stage/house.yaml";
    if (!LoadDebugStage(initialStageNumber, initialStageYamlPath)) {
        return false;
    }

    ClosePauseMenu();
    SetFreeCameraMode(false);
    return true;
}

bool Game::RestoreDebugEditorStage(int stageNum, const std::string& yamlPath)
{
    return LoadDebugStage(stageNum, yamlPath);
}

bool Game::LoadDebugStage(int stageNum, const std::string& yamlPath)
{
    if (yamlPath.empty() || stageNum < 0 || stageNum >= static_cast<int>(GetStages().size())) {
        return false;
    }

    return mSceneSystem->LoadStageIntoPlayingScene([&]() { return LoadStageForScene(stageNum, yamlPath); });
}

void Game::CheckGameControllerConnected()
{
    mGamepadRumbleService->UpdateConnection();
}

void Game::OnBoatStageChangeRequested(int destStage)
{
    if (GetCurrentStageNum() != 0) {
        return;
    }

    mSceneSystem->RequestStageChange(destStage);
}

void Game::OnBoatArrived(Boat* boat)
{
    mSceneSystem->OnBoatArrived(boat);

    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->MergeSoloSplitAfterBoatArrival();
    }
    mAudioSystem->TryChangeBGM();
}

void Game::OnPlayerCurrentPlanetChanged(Player& player)
{
    if (&player != GetMainPlayer() || !mAudioSystem) {
        return;
    }

    mAudioSystem->TryChangeBGM();
}

void Game::OnStarObtained()
{
    if (mUGCModeController->HandleGoalObtained()) {
        return;
    }
    MarkStageCleared(GetCurrentStageNum());
    mSceneSystem->OnStageClear();
}

void Game::OnStarCollectionAnimationFinished()
{
    if (mUGCModeController) {
        mUGCModeController->HandleGoalCollectionFinished();
    }
}

void Game::ForcePlayersGroundedForCinematic()
{
    for (Player* player : GetPlayers()) {
        if (player && player->GetIsActive()) {
            player->ForceGroundedForCinematic();
        }
    }
}

void Game::ProcessPendingUGCClearCompletion()
{
    mUGCModeController->ProcessPendingClearCompletion();
}

bool Game::IsStageCleared(int stageNum) const
{
    return mProgressController && mProgressController->IsStageCleared(stageNum);
}

ProgressUpdateResult Game::MarkStageCleared(int stageNum)
{
    return SetStageCleared(stageNum, true);
}

ProgressUpdateResult Game::SetStageCleared(int stageNum, bool isCleared)
{
    ProgressUpdateResult result;
    if (mProgressController) {
        result = mProgressController->SetStageCleared(stageNum, isCleared);
    }
    if (isCleared && mSessionController) {
        mSessionController->RecordStageCleared(stageNum);
    }
    return result;
}

bool Game::HasCompletedTutorial(const std::string& tutorialId) const
{
    return mProgressController && mProgressController->HasCompletedTutorial(tutorialId);
}

void Game::MarkTutorialCompleted(const std::string& tutorialId)
{
    if (mProgressController) {
        mProgressController->MarkTutorialCompleted(tutorialId);
    }
}

bool Game::HasShownNPCConversation(const NPC* npc) const
{
    return mProgressController && mProgressController->HasShownNPCConversation(npc);
}

void Game::MarkNPCConversationShown(const NPC* npc)
{
    if (mProgressController) {
        mProgressController->MarkNPCConversationShown(npc);
    }
}

bool Game::HasSeenBaseIntro() const
{
    return mProgressController && mProgressController->HasSeenBaseIntro();
}

void Game::MarkBaseIntroSeen()
{
    if (mProgressController) {
        mProgressController->MarkBaseIntroSeen();
    }
}

bool Game::HasCompletedNPCOpeningTrigger(const NPC* npc, std::size_t talkPageIndex) const
{
    return mProgressController && mProgressController->HasCompletedNPCOpeningTrigger(npc, talkPageIndex);
}

void Game::MarkNPCOpeningTriggerCompleted(const NPC* npc, std::size_t talkPageIndex)
{
    if (mProgressController) {
        mProgressController->MarkNPCOpeningTriggerCompleted(npc, talkPageIndex);
    }
}

bool Game::AreAllMainStagesCleared() const
{
    return mProgressController && mProgressController->AreAllMainStagesCleared();
}

bool Game::HasCompletedEndingRoll() const
{
    return mProgressController && mProgressController->HasCompletedEndingRoll();
}

void Game::MarkEndingRollCompleted()
{
    if (mProgressController) {
        mProgressController->MarkEndingRollCompleted();
    }
}

bool Game::HasCompletedNPCEndingTrigger(const NPC* npc, std::size_t talkPageIndex) const
{
    return mProgressController && mProgressController->HasCompletedNPCEndingTrigger(npc, talkPageIndex);
}

void Game::MarkNPCEndingTriggerCompleted(const NPC* npc, std::size_t talkPageIndex)
{
    if (mProgressController) {
        mProgressController->MarkNPCEndingTriggerCompleted(npc, talkPageIndex);
    }
}

void Game::OnEnemyLaunched()
{
    mAudioSystem->PlaySE("break_se");
    mSceneSystem->OnEnemyLaunched();
}

void Game::OnLanded()
{
    mSceneSystem->OnLanded();
}

void Game::OnPlayerDied()
{
    if (mSessionController) {
        mSessionController->RecordPlayerDeath();
    }
    mSceneSystem->OnPlayerDied();
}

void Game::OnBoatPartsObtained()
{
    mAudioSystem->PlaySE("pickup_se");

    Player* mainPlayer = GetMainPlayer();
    if (!mainPlayer || !mainPlayer->GetCurrentPlanet()) {
        return;
    }

    mainPlayer->GetCurrentPlanet()->OnBoatPartsObtained();
}

Player* Game::FindNearestPlayer(Actor* actor) const
{
    return mWorld->FindNearestPlayer(actor);
}

void Game::ReturnToTitleFromPauseMenu()
{
    mSessionController->ReturnToTitle();
}

void Game::RestartGame()
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->MergeSoloSplitBeforeRestart();
    }

    std::vector<Player*> playersToKeepInactive;
    for (Player* player : GetPlayers()) {
        if (!player) {
            continue;
        }

        if (!player->GetIsActive()) {
            playersToKeepInactive.emplace_back(player);
        }

        player->Restart();
    }

    // Restartで再有効化される待機中Playerは、再開前の非アクティブ状態へ戻す。
    for (Player* player : playersToKeepInactive) {
        player->SetVelocity(glm::vec3(0.0f));
        player->SetControlLocked(true);
        player->SetIsActive(false);
    }

    if (mCameraSystem) {
        mCameraSystem->SnapBehindControlledPlayer();
    }
}

void Game::StartPlayingScene()
{
    mSceneSystem->StartPlayingScene();
}

void Game::StartUGCStageClearPresentation()
{
    if (mSceneSystem) {
        mSceneSystem->OnUGCStageClear();
    }
}

void Game::StartFocusingScene()
{
    mSceneSystem->StartFocusingScene();
}

void Game::FinishFocusingScene()
{
    mSceneSystem->FinishFocusingScene();
}

void Game::OnPlayerApplyDamage(int playerNum)
{
    mAudioSystem->PlaySE("damaged_se");
    VibrateControllerForPlayer(playerNum, 0, 10000, 1000);
    if (mCameraSystem) {
        mCameraSystem->StartPlayerDamagedShake(playerNum);
    }
}

void Game::SynchronizeSoloSplitResources(const Player& sourcePlayer)
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->SynchronizeSoloSplitResources(sourcePlayer);
    }
}

void Game::OnPlayerAttackHit(int playerNum)
{
    VibrateControllerForPlayer(playerNum, 0, 10000, 200);
}

void Game::OnStrongAttacked(int playerNum)
{
    mSceneSystem->OnStrongAttacked();
    mHitStopTimer = 0.4f;
    VibrateControllerForPlayer(playerNum, 40000, 0, 500);
}

void Game::OnAirSlamAttackHit(int playerNum)
{
    OnStrongAttacked(playerNum);
    if (mCameraSystem) {
        mCameraSystem->StartAirStrongAttackHitShake(playerNum);
    }
}

void Game::OnPlayerCounter(int playerNum)
{
    mHitStopTimer = 0.3f;
    VibrateControllerForPlayer(playerNum, 25000, 0, 500);
}

void Game::VibrateControllerForPlayer(int playerNum, int lowFrequency, int highFrequency, int durationMilliseconds)
{
    mGamepadRumbleService->VibrateForPlayer(playerNum, lowFrequency, highFrequency, durationMilliseconds);
}

SDL_GameController* Game::GetSdlController() const
{
    return mGamepadRumbleService->GetController();
}

const std::vector<Player*>& Game::GetPlayers() const
{
    return mWorld->GetPlayers();
}

Player* Game::GetMainPlayer() const
{
    return mPlayerConfigurationController ? mPlayerConfigurationController->GetMainPlayer()
                                          : (mWorld ? mWorld->GetMainPlayer() : nullptr);
}

Player* Game::GetControlledPlayer() const
{
    return mPlayerConfigurationController ? mPlayerConfigurationController->GetControlledPlayer()
                                          : (mWorld ? mWorld->GetMainPlayer() : nullptr);
}

int Game::GetControlledPlayerIndex() const
{
    return mPlayerConfigurationController ? mPlayerConfigurationController->GetControlledPlayerIndex() : 0;
}

bool Game::GetIsPlayer2Joined() const
{
    return mPlayerConfigurationController && mPlayerConfigurationController->IsSecondPlayerJoined();
}

bool Game::GetIsPlayerSplit() const
{
    return mPlayerConfigurationController && mPlayerConfigurationController->IsPlayerSplit();
}

const std::vector<Stage*>& Game::GetStages() const
{
    return mWorld->GetStages();
}

Stage* Game::GetCurrentStage() const
{
    return mWorld->GetCurrentStage();
}

int Game::GetCurrentStageNum() const
{
    return mWorld->GetCurrentStageNum();
}

const std::string& Game::GetCurrentStageYamlPath() const
{
    return mStageFlowController->GetCurrentStageYamlPath();
}

bool Game::LoadStageForScene(int stageNum, const std::string& yamlPath)
{
    const int previousStageNum = mWorld->GetCurrentStageNum();
    const std::string previousYamlPath = mStageFlowController->GetCurrentStageYamlPath();
    if (yamlPath.empty() || stageNum < 0 || stageNum >= static_cast<int>(GetStages().size()) ||
        !mWorld->ChangeStage(stageNum)) {
        return false;
    }

    mStageFlowController->SetCurrentStageYamlPath(yamlPath);
    if (ReloadCurrentStage()) {
        return true;
    }
    mWorld->ChangeStage(previousStageNum);
    mStageFlowController->SetCurrentStageYamlPath(previousYamlPath);
    mPhysicsSystem->Initialize();
    return false;
}

std::string Game::GetNPCConversationId(const NPC* npc) const
{
    return mProgressController ? mProgressController->BuildNPCConversationId(npc) : std::string{};
}

NPC* Game::FindNPCByConversationId(const std::string& conversationId) const
{
    if (conversationId.empty()) {
        return nullptr;
    }

    Stage* stage = GetCurrentStage();
    if (!stage) {
        return nullptr;
    }

    for (Planet* planet : stage->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (NPC* npc : planet->GetNPCs()) {
            if (npc && mProgressController->BuildNPCConversationId(npc) == conversationId) {
                return npc;
            }
        }
    }
    return nullptr;
}

bool Game::GetIsPauseMenuOpen() const
{
    return mPauseMenuController->IsOpen();
}

int Game::GetPauseMenuSelectedIndex() const
{
    return mPauseMenuController->GetSelectedIndex();
}

bool Game::IsInBase() const
{
    return mWorld->IsInBase();
}

bool Game::IsGameControllerConnected() const
{
    return mGamepadRumbleService->IsConnected();
}

bool Game::HasGameControllerForPlayer(int playerNum) const
{
    return mGamepadRumbleService->HasControllerForPlayer(playerNum);
}
