#include <GL/glew.h>

#include "Game.h"
#include "Stage.h"

#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "state/GameProgressState.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
#include "system/DebugEditorSessionController.h"
#include "system/EnemyJewelDropSystem.h"
#include "system/CameraSystem.h"
#include "system/GameWorld.h"
#include "system/GameProgressController.h"
#include "system/GamepadRumbleService.h"
#include "system/InputSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/PauseMenuController.h"
#include "system/ParticleSystem.h"
#include "system/PlayerConfigurationController.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "system/StageFlowController.h"
#include "system/UILoadSystem.h"
#include "system/UserDataPaths.h"
#include "system/UGCModeController.h"
#include "system/sequence/SequenceSystem.h"

#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"
#include "gfx/GameFrameRenderer.h"
#include "gfx/debug/ugc/UGCPreviewController.h"
#include "imgui.h"

#include "utils/MathUtils.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iterator>
#include <iostream>

namespace {

std::string BuildStageIntroCinematicId(int stageNum)
{
    if (stageNum <= 0) {
        return {};
    }

    return "enter_stage" + std::to_string(stageNum);
}

}

Game::Game()
    : mWindow(nullptr),
      mHitStopTimer(-1.0f),
      mLastTime(0.0),
      mIsDebugEditorShowing(false),
      mIsFreeCameraMode(false),
      mIsDebugMode(false)
{
    mUGCModeController = std::make_unique<UGCModeController>(*this);
    mUGCPreviewController =
        std::make_unique<UGCPreviewController>(this);
}

Game::~Game() = default;

bool Game::Initialize(
    bool isDebugMode,
    const std::string& editorSessionPath,
    const std::string& editorRestartErrorLogPath)
{
    if (!InitializeGLFW()) {
        return false;
    }

    std::string userDataErrorMessage;
    if (!UserDataPaths::PrepareFromPackagedAssets(
            "../assets/data",
            userDataErrorMessage)) {
        std::cerr << userDataErrorMessage << '\n';
    }

    if (!CreateGameSystems()) {
        Shutdown();
        return false;
    }
    InitializeGameController();

    constexpr int stageCount = 6;
    CreateStages(stageCount);

    if (isDebugMode) {
        mIsDebugMode = true;
        mWorld->ChangeStage(1);
        mStageFlowController->SetCurrentStageYamlPath("../assets/data/stage/test.yaml");
        mSceneSystem->StartPlayingScene();
    }

    ReloadCurrentStage();
    RestoreDebugEditorSessionAtStartup(
        editorSessionPath,
        editorRestartErrorLogPath);

    mLastTime = glfwGetTime();
    glEnable(GL_DEPTH_TEST);

    return true;
}

bool Game::InitializeGLFW()
{
    if (!glfwInit()) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    mWindow = glfwCreateWindow(800, 450, "Slime'sSpaceTravel", nullptr, nullptr);
    if (!mWindow) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(mWindow);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
        return false;
    }

    return true;
}

void Game::InitializeGameController()
{
    mGamepadRumbleService->Initialize();
}

bool Game::CreateGameSystems()
{
    mWorld = std::make_unique<GameWorld>();
    mPauseMenuController = std::make_unique<PauseMenuController>();
    mStageFlowController = std::make_unique<StageFlowController>();
    mGamepadRumbleService = std::make_unique<GamepadRumbleService>();
    mEnemyJewelDropSystem =
        std::make_unique<EnemyJewelDropSystem>(this);

    mAudioSystem = std::make_unique<AudioSystem>(this);
    mUIRenderer = std::make_unique<UIRenderer>(this);
    if (!mUIRenderer->IsInitialized()) {
        std::cerr << "Failed to initialize the UI renderer.\n";
        return false;
    }
    mDebugEditorSessionController =
        std::make_unique<DebugEditorSessionController>(
            mIsDebugMode, *mWindow, *mUIRenderer);
    mRenderer3D = std::make_unique<Renderer3D>(this);
    if (!mRenderer3D->IsInitialized()) {
        std::cerr << "Failed to initialize the 3D renderer.\n";
        return false;
    }
    mSceneSystem = std::make_unique<SceneSystem>(
        this,
        *mUIRenderer->GetUILoadSystem());
    mMathUtils = std::make_unique<MathUtils>();
    mCameraSystem = std::make_unique<CameraSystem>(this);
    mMeshLoadSystem = std::make_unique<MeshLoadSystem>(this);
    mUGCPreviewController->SetMeshLoadSystem(*mMeshLoadSystem);
    mActorLoadSystem = std::make_unique<ActorLoadSystem>(this, *mWorld);
    mPhysicsSystem = std::make_unique<PhysicsSystem>(this);
    mProgressController = std::make_unique<GameProgressController>(
        *mWorld,
        *mPhysicsSystem,
        mStageFlowController->GetCurrentStageYamlPath());
    mProgressController->Load();
    if (mProgressController->HasSelectedPlayerControlStyle()) {
        mPlayerControlStyle =
            mProgressController->GetSelectedPlayerControlStyle();
    }
    mInputSystem = std::make_unique<InputSystem>(this);
    mSequenceSystem = std::make_unique<SequenceSystem>(this);
    mPlayerConfigurationController =
        std::make_unique<PlayerConfigurationController>(
            PlayerConfigurationDependencies{
                .world = *mWorld,
                .playerLoader = mActorLoadSystem->GetPlayerLoader(),
                .sceneSystem = *mSceneSystem,
                .cameraSystem = *mCameraSystem,
                .sequenceSystem = *mSequenceSystem,
                .progressController = *mProgressController,
                .gamepadService = *mGamepadRumbleService,
                .pauseMenuController = *mPauseMenuController,
            });

    mFrameRenderer = std::make_unique<GameFrameRenderer>(
        *mWindow,
        *mRenderer3D,
        *mUIRenderer,
        *mCameraSystem,
        *mUGCPreviewController,
        mFramePerformanceTracker);

    mParticleSystem = std::make_unique<ParticleSystem>();
    mParticleSystem->LoadDefinitions("../assets/data/effects/particles.yaml");
    return true;
}

void Game::CreateStages(int stageCount)
{
    mWorld->CreateStages(stageCount);
}

void Game::ReloadCurrentStage(StagePhysicsReloadMode physicsReloadMode)
{
    if (mSequenceSystem) {
        mSequenceSystem->Stop(true);
    }

    if (mCameraSystem) {
        mCameraSystem->ResetForStageChange();
    }

    if (mParticleSystem) {
        mParticleSystem->Clear();
    }

    if (physicsReloadMode == StagePhysicsReloadMode::SkipRebuild &&
        mPhysicsSystem) {
        mPhysicsSystem->ClearForEditorStageRebuild();
    }

    mStageFlowController->ReloadCurrentStage(*this, physicsReloadMode);

    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->SynchronizeAfterStageReload();
    }

    if (mCameraSystem) {
        mCameraSystem->SnapBehindControlledPlayer();
    }
}

void Game::ReloadUIData()
{
    if (!mUIRenderer) {
        return;
    }

    mUIRenderer->GetUILoadSystem()->Initialize();
}

void Game::RunLoop()
{
    while (!glfwWindowShouldClose(mWindow)) {
        const auto frameStartTime = std::chrono::steady_clock::now();
        mFramePerformanceTracker.BeginFrame();
        mFrameRenderer->PollGpuPerformanceMeasurements();
        glfwPollEvents();
        ProcessInput();

        const auto gameUpdateStartTime = std::chrono::steady_clock::now();
        UpdateGame();
        const auto gameUpdateEndTime = std::chrono::steady_clock::now();
        mFramePerformanceTracker.RecordGameUpdateDuration(
            std::chrono::duration<float, std::milli>(
                gameUpdateEndTime - gameUpdateStartTime).count());

        const GameFrameRenderState renderState{
            .isDebugEditorShowing = mIsDebugEditorShowing,
            .isUGCModeActive = GetIsUGCMode(),
            .isUGCWorkBrowserShowing = GetIsUGCWorkBrowserShowing(),
            .isUGCOrthographicView = GetIsUGCOrthographicView(),
            .deltaTimeSeconds = mLastDeltaTime,
        };
        mFrameRenderer->Render(renderState);

        mFramePerformanceTracker.RecordTotalDuration(
            std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - frameStartTime).count());
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

    if (mFrameRenderer) {
        mFrameRenderer->Shutdown();
        mFrameRenderer.reset();
    }
    if (mRenderer3D) {
        mRenderer3D->Shutdown();
        mRenderer3D.reset();
    }
    if (mUIRenderer) {
        mUIRenderer->Shutdown();
        mUIRenderer.reset();
    }

    SDL_Quit();

    if (mWindow) {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }

    glfwTerminate();
}

void Game::RestoreDebugEditorSessionAtStartup(
    const std::string& editorSessionPath,
    const std::string& editorRestartErrorLogPath)
{
    if (!mDebugEditorSessionController) {
        return;
    }

    const DebugEditorSessionRestoreOutcome restoreOutcome =
        mDebugEditorSessionController->RestoreAtStartup(
            editorSessionPath, editorRestartErrorLogPath);
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
        mInputSystem->CaptureFrameInput();
        mInputSystem->ProcessGameInput();
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
    return mSceneSystem &&
           mSceneSystem->IsPlaying() &&
           mCameraSystem &&
           mCameraSystem->AllowsPlayerInput() &&
           (!mSequenceSystem || !mSequenceSystem->LocksPlayerControl());
}

void Game::ExecutePauseMenuItem()
{
    mPauseMenuController->ExecuteSelectedItem(*this);
}

void Game::ReturnToBase()
{
    mStageFlowController->ReturnToBase(*this);
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

void Game::ReturnToSinglePlayer()
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->ReturnToSinglePlayer();
    }
}

bool Game::CanStartTwoPlayerFromPauseMenu() const
{
    return mPlayerConfigurationController &&
        mPlayerConfigurationController
        ->CanStartTwoPlayerFromPauseMenu();
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
        outErrorMessage =
            "The editor build restart service is not available.";
        return false;
    }
    return mDebugEditorSessionController->RequestBuildAndRestart(
        outErrorMessage);
}

void Game::ProcessActorsInput()
{
    const bool isWaitingForTutorialPlayerJump =
        mSceneSystem->IsWaitingForTutorialPlayerJump();
    const bool allowsPlayerControl =
        (mSceneSystem->IsPlaying() ||
         isWaitingForTutorialPlayerJump) &&
        !IsEditorKeyboardInputCaptured() &&
        mCameraSystem->AllowsPlayerInput() &&
        (!mSequenceSystem || !mSequenceSystem->LocksPlayerControl());

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

void Game::RecordViewportRenderDurationMilliseconds(
    int viewportIndex,
    float durationMilliseconds)
{
    mFramePerformanceTracker.RecordViewportCpuDuration(
        viewportIndex, durationMilliseconds);
}

void Game::RecordViewportGpuDurationMilliseconds(
    int viewportIndex,
    float durationMilliseconds)
{
    mFramePerformanceTracker.RecordViewportGpuDuration(
        viewportIndex, durationMilliseconds);
}

const FramePerformanceMetrics& Game::GetFramePerformanceMetrics() const
{
    return mFramePerformanceTracker.GetMetrics();
}

void Game::UpdateGame()
{
    CheckGameControllerConnected();

    const double currentTime = glfwGetTime();
    const float deltaTime = std::min(0.04f, static_cast<float>(currentTime - mLastTime));
    mLastTime = currentTime;
    mLastDeltaTime = deltaTime;

    ProcessPendingUGCClearCompletion();

    if (mHitStopTimer >= 0.0f) {
        mHitStopTimer -= deltaTime;
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

    bool cameraUpdated = false;
    const bool shouldUpdateEntireWorld =
        mSceneSystem->CanUpdateWorld() || mSceneSystem->IsStageClear();
    const bool shouldUpdateTutorialPlayer =
        mSceneSystem->IsWaitingForTutorialPlayerJump();

    if (shouldUpdateEntireWorld) {
        UpdateActors(deltaTime);

        if (mParticleSystem) {
            mParticleSystem->Update(deltaTime);
        }
    } else if (shouldUpdateTutorialPlayer) {
        mWorld->UpdatePlayer(GetControlledPlayer(), deltaTime);
    }

    if (mSequenceSystem) {
        mSequenceSystem->Update(deltaTime);
    }

    if (shouldUpdateEntireWorld ||
        shouldUpdateTutorialPlayer) {
        mCameraSystem->Update(deltaTime);
        cameraUpdated = true;
    }

    if (!cameraUpdated && (mCameraSystem->IsCinematicPlaying() || mSceneSystem->IsTalking())) {
        mCameraSystem->Update(deltaTime);
    }

}

void Game::UpdateActors(float deltaTime)
{
    mWorld->UpdateActors(deltaTime);
    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->SpawnPendingDrops();
    }
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

void Game::SetUGCPlatformPlacementPreview(
    const std::optional<glm::vec3>& position)
{
    mUGCPreviewController->SetPlatformPlacementPosition(position);
}

const std::optional<glm::vec3>&
Game::GetUGCPlatformPlacementPreview() const
{
    return mUGCPreviewController->GetPlatformPlacementPosition();
}

void Game::SetUGCMovingPlatformPathPreview(
    const std::optional<glm::vec3>& startPosition,
    const std::optional<glm::vec3>& destinationPosition)
{
    mUGCPreviewController->SetMovingPlatformPath(
        startPosition, destinationPosition);
}

const std::optional<glm::vec3>&
Game::GetUGCMovingPlatformPathStartPosition() const
{
    return mUGCPreviewController->GetMovingPlatformPathStart();
}

const std::optional<glm::vec3>&
Game::GetUGCMovingPlatformPathDestinationPosition() const
{
    return mUGCPreviewController->GetMovingPlatformPathDestination();
}

void Game::SetUGCOrthographicHalfHeight(float halfHeight)
{
    mUGCPreviewController->SetOrthographicHalfHeight(halfHeight);
}

float Game::GetUGCOrthographicHalfHeight() const
{
    return mUGCPreviewController->GetOrthographicHalfHeight();
}

void Game::SetUGCGridSize(float gridSize)
{
    mUGCPreviewController->SetGridSize(gridSize);
}

float Game::GetUGCGridSize() const
{
    return mUGCPreviewController->GetGridSize();
}

unsigned int Game::GetUGCPreviewTexture() const
{
    return mFrameRenderer
        ? mFrameRenderer->GetUGCPreviewTexture()
        : 0;
}

bool Game::GetIsUGCEditorTutorialActive() const
{
    return mUGCModeController &&
        mUGCModeController->IsEditorTutorialActive();
}

bool Game::GetIsUGCWorkBrowserShowing() const
{
    return mUGCModeController &&
        mUGCModeController->IsWorkBrowserShowing();
}

bool Game::GetIsUGCMode() const
{
    return mUGCModeController && mUGCModeController->IsModeActive();
}

bool Game::GetIsUGCPlaytestActive() const
{
    return mUGCModeController &&
        mUGCModeController->IsPlaytestActive();
}

bool Game::GetIsUGCClearVerificationActive() const
{
    return mUGCModeController &&
        mUGCModeController->IsVerificationActive();
}

bool Game::GetIsUGCDebugEditorShowing() const
{
    return mUGCModeController &&
        mUGCModeController->IsDebugPanelShowing();
}

bool Game::GetIsUGCOrthographicView() const
{
    return mUGCModeController &&
        mUGCModeController->IsOrthographicView();
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

void Game::SetUGCPlacementModelPreview(
    const std::optional<glm::vec3>& position,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath)
{
    mUGCPreviewController->SetPlacementModel(
        position, modelPath, scale, textureOverridePath);
}

void Game::SetUGCPlacementModelPreviewPositions(
    const std::vector<glm::vec3>& positions,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath)
{
    mUGCPreviewController->SetPlacementModelPositions(
        positions, modelPath, scale, textureOverridePath);
}

Actor* Game::GetUGCPlacementModelPreview() const
{
    return mUGCPreviewController->GetPlacementModel();
}

const std::vector<glm::vec3>&
Game::GetUGCPlacementModelPreviewPositions() const
{
    return mUGCPreviewController->GetPlacementModelPositions();
}

void Game::RequestEnemyJewelDrop(
    const Enemy& defeatedEnemy)
{
    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->RequestDrop(
            defeatedEnemy);
    }
}

const std::vector<JewelItem*>&
Game::GetRuntimeJewelItems() const
{
    static const std::vector<JewelItem*> emptyItems;
    return mEnemyJewelDropSystem
        ? mEnemyJewelDropSystem->GetRuntimeItems()
        : emptyItems;
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
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->ToggleSplit();
}

bool Game::CanTogglePlayerSplit() const
{
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->CanToggleSplit();
}

bool Game::SwitchControlledPlayer()
{
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->SwitchControlledPlayer();
}

bool Game::CanSwitchControlledPlayer() const
{
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->CanSwitchControlledPlayer();
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

void Game::LoadData()
{
    mStageFlowController->LoadData(*this);
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

    const std::string cinematicId =
        BuildStageIntroCinematicId(stageNum);
    if (cinematicId.empty()) {
        return false;
    }

    return mCameraSystem->GetCinematicLibrary().Find(cinematicId) != nullptr;
}

bool Game::StartStageIntroCinematic(int stageNum)
{
    if (!mSequenceSystem || !mAudioSystem ||
        !HasStageIntroCinematic(stageNum)) {
        return false;
    }

    const std::string cinematicId =
        BuildStageIntroCinematicId(stageNum);
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

    const bool isBaseStageYaml =
        yamlPath.ends_with("/stage0.yaml") || yamlPath.ends_with("\\stage0.yaml");
    if (isBaseStageYaml && mSequenceSystem) {
        if (!HasSeenBaseIntro() &&
            mSequenceSystem->PlayCinematicChainThenSequence(
                {"base_sequence"}, "base_arrival_template")) {
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
    mTitleMenuSelection = 0;
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

bool Game::RequestSceneFadeAction(
    const std::function<void()>& fadeAction)
{
    return mSceneSystem &&
           mSceneSystem->RequestFadeAction(fadeAction);
}

void Game::NotifyUGCTutorialReturnedFromPlaytest()
{
    if (mUIRenderer) {
        mUIRenderer->NotifyUGCEditorTutorialReturnedFromPlaytest();
    }
}

void Game::CompleteUGCVerification(const std::string& workFileName)
{
    if (mUIRenderer) {
        mUIRenderer->CompleteUGCVerification(workFileName);
    }
}

bool Game::HasProgressFlag(const std::string& progressId) const
{
    return mProgressController &&
           mProgressController->HasProgressFlag(progressId);
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

void Game::OpenUGCWorkBrowser()
{
    mUGCModeController->OpenWorkBrowser();
}

void Game::MoveTitleMenuSelection(int delta)
{
    if (!mSceneSystem || !mSceneSystem->IsTitle() || delta == 0) {
        return;
    }

    constexpr int titleMenuItemCount = 3;
    mTitleMenuSelection =
        (mTitleMenuSelection + delta + titleMenuItemCount) %
        titleMenuItemCount;
}

void Game::ExecuteTitleMenuSelection()
{
    if (!mSceneSystem || !mSceneSystem->IsTitle()) {
        return;
    }

    switch (mTitleMenuSelection) {
    case 0:
        mSceneSystem->StartBattleStyleSelection();
        break;
    case 1:
        mSceneSystem->RequestFadeAction([this]() {
            mUGCModeController->StartModeFromTitleSelection();
        });
        break;
    case 2:
        OpenUGCWorkBrowser();
        break;
    default:
        break;
    }
}

void Game::CloseUGCWorkBrowser()
{
    mUGCModeController->CloseWorkBrowser();
}

void Game::StartUGCPlaytest()
{
    mUGCModeController->StartPlaytest();
}

void Game::UndoUGCEdit()
{
    if (mUIRenderer) mUIRenderer->UndoUGCEdit();
}

void Game::RedoUGCEdit()
{
    if (mUIRenderer) mUIRenderer->RedoUGCEdit();
}

void Game::ToggleUGCEraser()
{
    if (mUIRenderer) mUIRenderer->ToggleUGCEraser();
}

void Game::SelectUGCEditorMode()
{
    if (mUIRenderer) mUIRenderer->SelectUGCEditorMode();
}

void Game::OpenUGCEditorMenu()
{
    if (mUIRenderer) mUIRenderer->OpenUGCEditorMenu();
}

void Game::ZoomUGCEditor(float distanceMultiplier)
{
    if (mUIRenderer) mUIRenderer->ZoomUGCEditor(distanceMultiplier);
}

void Game::ChangeUGCEditLayer(int layerDelta)
{
    if (mUIRenderer) mUIRenderer->ChangeUGCEditLayer(layerDelta);
}

void Game::MoveUGCSelectionByGrid(int gridX, int gridZ)
{
    if (mUIRenderer) {
        mUIRenderer->MoveUGCSelectionByGrid(gridX, gridZ);
    }
}

void Game::StartUGCClearVerification(
    const std::string& workFileName)
{
    mUGCModeController->StartClearVerification(workFileName);
}

void Game::ReturnToUGCEditor()
{
    mUGCModeController->ReturnToEditor();
}

void Game::ExitUGCMode()
{
    mUGCModeController->ExitMode();
}

bool Game::PrepareInitialSceneForDebug()
{
    if (!mIsDebugMode) {
        return false;
    }

    constexpr int InitialStageNumber = 0;
    constexpr const char* InitialStageYamlPath =
        "../assets/data/stage/house.yaml";
    if (!LoadDebugStage(
            InitialStageNumber,
            InitialStageYamlPath)) {
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
    if (yamlPath.empty() ||
        stageNum < 0 ||
        stageNum >= static_cast<int>(GetStages().size())) {
        return false;
    }

    if (!mWorld->ChangeStage(stageNum)) {
        return false;
    }

    mStageFlowController->SetCurrentStageYamlPath(yamlPath);
    mSceneSystem->StartPlayingScene();
    ReloadCurrentStage();
    return true;
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
    return mProgressController &&
           mProgressController->IsStageCleared(stageNum);
}

void Game::MarkStageCleared(int stageNum)
{
    SetStageCleared(stageNum, true);
}

void Game::SetStageCleared(int stageNum, bool isCleared)
{
    if (mProgressController) {
        mProgressController->SetStageCleared(stageNum, isCleared);
    }
}

bool Game::HasCompletedTutorial(const std::string& tutorialId) const
{
    return mProgressController &&
           mProgressController->HasCompletedTutorial(tutorialId);
}

void Game::MarkTutorialCompleted(const std::string& tutorialId)
{
    if (mProgressController) {
        mProgressController->MarkTutorialCompleted(tutorialId);
    }
}

bool Game::HasShownNPCConversation(const NPC* npc) const
{
    return mProgressController &&
           mProgressController->HasShownNPCConversation(npc);
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

bool Game::HasCompletedNPCOpeningTrigger(
    const NPC* npc, std::size_t talkPageIndex) const
{
    return mProgressController &&
           mProgressController->HasCompletedNPCOpeningTrigger(
               npc, talkPageIndex);
}

void Game::MarkNPCOpeningTriggerCompleted(
    const NPC* npc, std::size_t talkPageIndex)
{
    if (mProgressController) {
        mProgressController->MarkNPCOpeningTriggerCompleted(
            npc, talkPageIndex);
    }
}

bool Game::AreAllMainStagesCleared() const
{
    return mProgressController &&
           mProgressController->AreAllMainStagesCleared();
}

bool Game::HasCompletedEndingRoll() const
{
    return mProgressController &&
           mProgressController->HasCompletedEndingRoll();
}

void Game::MarkEndingRollCompleted()
{
    if (mProgressController) {
        mProgressController->MarkEndingRollCompleted();
    }
}

bool Game::HasCompletedNPCEndingTrigger(
    const NPC* npc, std::size_t talkPageIndex) const
{
    return mProgressController &&
           mProgressController->HasCompletedNPCEndingTrigger(
               npc, talkPageIndex);
}

void Game::MarkNPCEndingTriggerCompleted(
    const NPC* npc, std::size_t talkPageIndex)
{
    if (mProgressController) {
        mProgressController->MarkNPCEndingTriggerCompleted(
            npc, talkPageIndex);
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

void Game::FinishGame()
{



    if (mProgressController) {
        mProgressController->Save();
    }
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void Game::RestartGame()
{



    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->MergeSoloSplitBeforeRestart();
    }

    std::vector<Player*> playersToKeepInactive;
    for (auto player : GetPlayers()) {
        if (!player) {
            continue;
        }

        if (!player->GetIsActive()) {
            playersToKeepInactive.emplace_back(player);
        }



        player->Restart();
    }

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

void Game::StartFocusingScene()
{
    mSceneSystem->StartFocusingScene();
}

void Game::OnPlayerApplyDamage(int playerNum)
{
    mAudioSystem->PlaySE("damaged_se");
    VibrateControllerForPlayer(playerNum, 0, 10000, 1000);
}

void Game::SynchronizeSoloSplitResources(const Player& sourcePlayer)
{
    if (mPlayerConfigurationController) {
        mPlayerConfigurationController->SynchronizeSoloSplitResources(
            sourcePlayer);
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

SDL_GameController* Game::GetSdlControllerForPlayer(int playerNum) const
{
    return mGamepadRumbleService->GetControllerForPlayer(playerNum);
}

const std::vector<Player*>& Game::GetPlayers() const
{
    return mWorld->GetPlayers();
}

Player* Game::GetMainPlayer() const
{
    return mPlayerConfigurationController
        ? mPlayerConfigurationController->GetMainPlayer()
        : (mWorld ? mWorld->GetMainPlayer() : nullptr);
}

Player* Game::GetControlledPlayer() const
{
    return mPlayerConfigurationController
        ? mPlayerConfigurationController->GetControlledPlayer()
        : (mWorld ? mWorld->GetMainPlayer() : nullptr);
}

int Game::GetControlledPlayerIndex() const
{
    return mPlayerConfigurationController
        ? mPlayerConfigurationController->GetControlledPlayerIndex()
        : 0;
}

bool Game::GetIsPlayer2Joined() const
{
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->IsSecondPlayerJoined();
}

bool Game::GetIsPlayerSplit() const
{
    return mPlayerConfigurationController &&
           mPlayerConfigurationController->IsPlayerSplit();
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
    if (yamlPath.empty() || stageNum < 0 ||
        stageNum >= static_cast<int>(GetStages().size()) ||
        !mWorld->ChangeStage(stageNum)) {
        return false;
    }

    mStageFlowController->SetCurrentStageYamlPath(yamlPath);
    ReloadCurrentStage();
    return true;
}

std::string Game::GetNPCConversationId(const NPC* npc) const
{
    return mProgressController
        ? mProgressController->BuildNPCConversationId(npc)
        : std::string{};
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
            if (npc &&
                mProgressController->BuildNPCConversationId(npc) ==
                    conversationId) {
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
