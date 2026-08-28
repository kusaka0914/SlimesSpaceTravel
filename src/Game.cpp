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
#include "system/EditorBuildRestartService.h"
#include "system/EnemyJewelDropSystem.h"
#include "system/CameraSystem.h"
#include "system/GameWorld.h"
#include "system/GamepadRumbleService.h"
#include "system/InputSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/PauseMenuController.h"
#include "system/ParticleSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "system/StageFlowController.h"
#include "system/StageProgressSystem.h"
#include "system/UILoadSystem.h"
#include "system/UserDataPaths.h"
#include "system/sequence/SequenceSystem.h"

#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"
#include "gfx/performance/GpuDurationTimer.h"
#include "imgui.h"

#include "utils/MathUtils.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <iostream>

namespace {
constexpr float playerMergeMaximumDistanceWorldUnits = 1.25f;

std::string BuildStageIntroCinematicId(int stageNum)
{
    if (stageNum <= 0) {
        return {};
    }

    return "enter_stage" + std::to_string(stageNum);
}

std::string BuildCompletedTutorialId(const std::string& tutorialId)
{
    return tutorialId.empty()
               ? std::string{}
               : "tutorial:completed:" + tutorialId;
}
}

Game::Game()
    : mWindow(nullptr),
      mHitStopTimer(-1.0f),
      mLastTime(0.0),
      mIsPlayer2Joined(false),
      mIsPlayerSplit(false),
      mIsDebugEditorShowing(false),
      mIsFreeCameraMode(false),
      mIsDebugMode(false)
{
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

    CreateGameSystems();
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

void Game::CreateGameSystems()
{
    mWorld = std::make_unique<GameWorld>();
    mPauseMenuController = std::make_unique<PauseMenuController>();
    mStageFlowController = std::make_unique<StageFlowController>();
    mStageProgressSystem = std::make_unique<StageProgressSystem>();
    mStageProgressSystem->Load();
    if (mStageProgressSystem->HasSelectedPlayerControlStyle()) {
        mPlayerControlStyle =
            mStageProgressSystem->IsAssistControlStyleSelected()
                ? PlayerControlStyle::Assist
                : PlayerControlStyle::Standard;
    }
    mGamepadRumbleService = std::make_unique<GamepadRumbleService>();
    mEditorBuildRestartService = std::make_unique<EditorBuildRestartService>();
    mEnemyJewelDropSystem =
        std::make_unique<EnemyJewelDropSystem>(this);

    mAudioSystem = std::make_unique<AudioSystem>(this);
    mUIRenderer = std::make_unique<UIRenderer>(this);
    mRenderer3D = std::make_unique<Renderer3D>(this);
    mGameUiGpuTimer = std::make_unique<GpuDurationTimer>();
    mEditorUiGpuTimer = std::make_unique<GpuDurationTimer>();
    mSceneSystem = std::make_unique<SceneSystem>(this);
    mMathUtils = std::make_unique<MathUtils>();
    mCameraSystem = std::make_unique<CameraSystem>(this);
    mMeshLoadSystem = std::make_unique<MeshLoadSystem>(this);
    mActorLoadSystem = std::make_unique<ActorLoadSystem>(this);
    mPhysicsSystem = std::make_unique<PhysicsSystem>(this);
    mInputSystem = std::make_unique<InputSystem>(this);
    mSequenceSystem = std::make_unique<SequenceSystem>(this);

    mParticleSystem = std::make_unique<ParticleSystem>();
    mParticleSystem->LoadDefinitions("../assets/data/effects/particles.yaml");
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

    if (mIsPlayer2Joined) {
        const std::vector<Player*>& players = GetPlayers();
        if (players.size() >= 2 && players[0] && players[1]) {
            Player* firstPlayer = players[0];
            Player* secondPlayer = players[1];



            secondPlayer->SetCurrentPlanet(firstPlayer->GetCurrentPlanet());
            secondPlayer->SetCurrentPlanetNum(firstPlayer->GetCurrentPlanetNum());
            secondPlayer->SetSphericalPlacement(
                firstPlayer->GetTheta(),
                firstPlayer->GetPhi(),
                firstPlayer->GetHeight());
            secondPlayer->SetOrientation(firstPlayer->GetOrientation());
            secondPlayer->SetFacingForwardVec(
                firstPlayer->GetFacingForwardVec());
            secondPlayer->SetCameraForwardDirection(
                -firstPlayer->GetFacingForwardVec(),
                firstPlayer->GetUpVec());
            secondPlayer->SetCameraYaw(firstPlayer->GetCameraYaw());
            secondPlayer->SetPos(firstPlayer->GetPos());
            secondPlayer->SetVelocity(glm::vec3(0.0f));
            secondPlayer->SetOnGround(firstPlayer->GetOnGround());
            secondPlayer->SetShouldJudgeLanding(!firstPlayer->GetOnGround());
            secondPlayer->RefreshFallbackUpVec();
        }
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
        BeginFramePerformanceMeasurement();
        PollGpuPerformanceMeasurements();
        glfwPollEvents();
        ProcessInput();

        const auto gameUpdateStartTime = std::chrono::steady_clock::now();
        UpdateGame();
        const auto gameUpdateEndTime = std::chrono::steady_clock::now();
        mFramePerformanceMetrics.gameUpdateMilliseconds =
            std::chrono::duration<float, std::milli>(
                gameUpdateEndTime - gameUpdateStartTime).count();

        GenerateOutput();

        mFramePerformanceMetrics.totalMilliseconds =
            std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - frameStartTime).count();
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

    if (mRenderer3D) {
        mRenderer3D->Shutdown();
    }
    if (mUIRenderer) {
        mUIRenderer->Shutdown();
        mUIRenderer.reset();
    }
    if (mGameUiGpuTimer) {
        mGameUiGpuTimer->Shutdown();
    }
    if (mEditorUiGpuTimer) {
        mEditorUiGpuTimer->Shutdown();
    }

    SDL_Quit();

    DestroyEditorGameRenderTarget();
    DestroyUGCPreviewRenderTarget();

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
    if (!mIsDebugMode || !mEditorBuildRestartService || !mUIRenderer) {
        return;
    }

    const bool hasExplicitRestartSession = !editorSessionPath.empty();
    std::filesystem::path sessionFilePath = editorSessionPath;
    if (!hasExplicitRestartSession) {
        std::string pathErrorMessage;
        if (!mEditorBuildRestartService->ResolvePersistentDebugSessionFilePath(
                sessionFilePath,
                pathErrorMessage)) {
            std::cerr << pathErrorMessage << std::endl;
            return;
        }

        std::error_code existsError;
        if (!std::filesystem::is_regular_file(sessionFilePath, existsError)) {
            return;
        }
    }

    std::string restoreErrorMessage;
    if (!mUIRenderer->RestoreDebugEditorSession(
            sessionFilePath.string(),
            restoreErrorMessage)) {
        std::cerr << restoreErrorMessage << std::endl;
        mIsDebugEditorShowing = true;
        mUIRenderer->SetEditorRestartStatus(restoreErrorMessage, true);
        return;
    }

    if (!editorRestartErrorLogPath.empty()) {
        mUIRenderer->SetEditorRestartStatus(
            "Build failed. See log: " + editorRestartErrorLogPath,
            true);
        return;
    }

    if (hasExplicitRestartSession) {
        mUIRenderer->SetEditorRestartStatus(
            "Build completed. The editor session was restored.",
            false);
        std::error_code removeError;
        std::filesystem::remove(sessionFilePath, removeError);
        return;
    }

    mUIRenderer->SetEditorRestartStatus(
        "The previous debug session was restored.",
        false);
}

void Game::SavePersistentDebugEditorSession()
{
    if (!mIsDebugMode || !mEditorBuildRestartService || !mUIRenderer) {
        return;
    }

    std::filesystem::path sessionFilePath;
    std::string saveErrorMessage;
    if (!mEditorBuildRestartService->ResolvePersistentDebugSessionFilePath(
            sessionFilePath,
            saveErrorMessage) ||
        !mUIRenderer->SaveDebugEditorSession(
            sessionFilePath.string(),
            saveErrorMessage)) {
        std::cerr << saveErrorMessage << std::endl;
    }
}

void Game::ProcessInput()
{
    SDL_PumpEvents();
    SDL_GameControllerUpdate();

    if (mInputSystem) {
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
    CreatePlayer2();
}

void Game::ReturnToSinglePlayer()
{
    if (!mIsPlayer2Joined) {
        return;
    }



    mIsPlayer2Joined = false;
    MergePlayerInto(0);
}

bool Game::CanStartTwoPlayerFromPauseMenu() const
{



    return mIsPlayer2Joined ||
           (IsStageCleared(1) && IsGameControllerConnected());
}

bool Game::CanReturnToBaseFromPauseMenu() const
{
    return IsStageCleared(1) && !IsInBase();
}

void Game::ToggleDebugEditor()
{
    if (mUGCSessionState.IsModeActive()) {



        mUGCSessionState.ToggleDebugPanel();
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
    outErrorMessage.clear();
    if (!mEditorBuildRestartService || !mUIRenderer || !mWindow) {
        outErrorMessage = "The editor build restart service is not available.";
        return false;
    }

    std::filesystem::path sessionFilePath;
    if (!mEditorBuildRestartService->ResolveSessionFilePath(
            sessionFilePath,
            outErrorMessage)) {
        return false;
    }

    if (!mUIRenderer->SaveDebugEditorSession(
            sessionFilePath.string(),
            outErrorMessage)) {
        return false;
    }

    if (!mEditorBuildRestartService->LaunchBuildAndRestartHelper(
            sessionFilePath,
            outErrorMessage)) {
        return false;
    }

    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
    return true;
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

void Game::BeginFramePerformanceMeasurement()
{
    mFramePerformanceMetrics.gameUpdateMilliseconds = 0.0f;
    mFramePerformanceMetrics.firstViewportRenderMilliseconds = 0.0f;
    mFramePerformanceMetrics.secondViewportRenderMilliseconds = 0.0f;
    mFramePerformanceMetrics.gameUiCpuMilliseconds = 0.0f;
    mFramePerformanceMetrics.editorUiCpuMilliseconds = 0.0f;
    mFramePerformanceMetrics.presentationWaitMilliseconds = 0.0f;
    mFramePerformanceMetrics.renderedViewportCount = 0;
}

void Game::PollGpuPerformanceMeasurements()
{
    if (mGameUiGpuTimer) {
        const std::optional<float> elapsedMilliseconds =
            mGameUiGpuTimer->PollCompletedMilliseconds();
        if (elapsedMilliseconds) {
            mFramePerformanceMetrics.gameUiGpuMilliseconds =
                *elapsedMilliseconds;
            mFramePerformanceMetrics.hasGameUiGpuMeasurement = true;
        }
    }

    if (mEditorUiGpuTimer) {
        const std::optional<float> elapsedMilliseconds =
            mEditorUiGpuTimer->PollCompletedMilliseconds();
        if (elapsedMilliseconds) {
            mFramePerformanceMetrics.editorUiGpuMilliseconds =
                *elapsedMilliseconds;
            mFramePerformanceMetrics.hasEditorUiGpuMeasurement = true;
        }
    }
}

void Game::RecordViewportRenderDurationMilliseconds(
    int viewportIndex,
    float durationMilliseconds)
{
    if (viewportIndex == 0) {
        mFramePerformanceMetrics.firstViewportRenderMilliseconds =
            durationMilliseconds;
    } else if (viewportIndex == 1) {
        mFramePerformanceMetrics.secondViewportRenderMilliseconds =
            durationMilliseconds;
    } else {
        return;
    }

    mFramePerformanceMetrics.renderedViewportCount = std::max(
        mFramePerformanceMetrics.renderedViewportCount,
        viewportIndex + 1);
}

void Game::RecordViewportGpuDurationMilliseconds(
    int viewportIndex,
    float durationMilliseconds)
{
    if (viewportIndex == 0) {
        mFramePerformanceMetrics.firstViewportGpuMilliseconds =
            durationMilliseconds;
        mFramePerformanceMetrics.hasFirstViewportGpuMeasurement = true;
    } else if (viewportIndex == 1) {
        mFramePerformanceMetrics.secondViewportGpuMilliseconds =
            durationMilliseconds;
        mFramePerformanceMetrics.hasSecondViewportGpuMeasurement = true;
    }
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

void Game::GenerateOutput()
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(
        mWindow,
        &framebufferWidth,
        &framebufferHeight);

    const bool shouldRenderEditorGameView =
        mIsDebugEditorShowing &&
        EnsureEditorGameRenderTarget(framebufferWidth, framebufferHeight);
    if (shouldRenderEditorGameView) {
        glBindFramebuffer(GL_FRAMEBUFFER, mEditorGameFramebuffer);
        DrawGameFrame();

        if (mUGCSessionState.IsModeActive() && EnsureUGCPreviewRenderTarget()) {
            glBindFramebuffer(GL_FRAMEBUFFER, mUGCPreviewFramebuffer);
            DrawUGCPreviewFrame();
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.035f, 0.035f, 0.045f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto editorUiStartTime = std::chrono::steady_clock::now();
        mEditorUiGpuTimer->Begin();
        mUIRenderer->DrawDebugEditor(
            mEditorGameTexture,
            framebufferWidth,
            framebufferHeight);
        mEditorUiGpuTimer->End();
        mFramePerformanceMetrics.editorUiCpuMilliseconds =
            std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - editorUiStartTime).count();
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DrawGameFrame();
        if (mIsDebugEditorShowing) {
            const auto editorUiStartTime = std::chrono::steady_clock::now();
            mEditorUiGpuTimer->Begin();
            mUIRenderer->DrawDebugEditor(
                0,
                framebufferWidth,
                framebufferHeight);
            mEditorUiGpuTimer->End();
            mFramePerformanceMetrics.editorUiCpuMilliseconds =
                std::chrono::duration<float, std::milli>(
                    std::chrono::steady_clock::now() - editorUiStartTime).count();
        } else if (mUGCSessionState.IsWorkBrowserShowing()) {
            mUIRenderer->DrawUGCWorkBrowser();
        }
    }

    const auto presentationStartTime = std::chrono::steady_clock::now();
    glfwSwapBuffers(mWindow);
    mFramePerformanceMetrics.presentationWaitMilliseconds =
        std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - presentationStartTime).count();
}

void Game::DrawGameFrame()
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(
        mWindow,
        &framebufferWidth,
        &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mUIRenderer->DrawSkyBox();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    mRenderer3D->Draw();

    glDisable(GL_DEPTH_TEST);
    const auto gameUiStartTime = std::chrono::steady_clock::now();
    mGameUiGpuTimer->Begin();
    mUIRenderer->DrawGameContent();
    mGameUiGpuTimer->End();
    mFramePerformanceMetrics.gameUiCpuMilliseconds =
        std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - gameUiStartTime).count();
    glEnable(GL_DEPTH_TEST);
}

bool Game::EnsureEditorGameRenderTarget(int width, int height)
{
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (mEditorGameFramebuffer == 0) {
        glGenFramebuffers(1, &mEditorGameFramebuffer);
        glGenTextures(1, &mEditorGameTexture);
        glGenRenderbuffers(1, &mEditorGameDepthBuffer);
    }

    const bool sizeChanged =
        width != mEditorGameRenderWidth ||
        height != mEditorGameRenderHeight;
    if (!sizeChanged) {
        return true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mEditorGameFramebuffer);

    glBindTexture(GL_TEXTURE_2D, mEditorGameTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        mEditorGameTexture,
        0);

    glBindRenderbuffer(GL_RENDERBUFFER, mEditorGameDepthBuffer);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height);
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        mEditorGameDepthBuffer);

    const bool isComplete =
        glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (!isComplete) {
        DestroyEditorGameRenderTarget();
        return false;
    }

    mEditorGameRenderWidth = width;
    mEditorGameRenderHeight = height;
    return true;
}

void Game::DestroyEditorGameRenderTarget()
{
    if (mEditorGameDepthBuffer != 0) {
        glDeleteRenderbuffers(1, &mEditorGameDepthBuffer);
        mEditorGameDepthBuffer = 0;
    }
    if (mEditorGameTexture != 0) {
        glDeleteTextures(1, &mEditorGameTexture);
        mEditorGameTexture = 0;
    }
    if (mEditorGameFramebuffer != 0) {
        glDeleteFramebuffers(1, &mEditorGameFramebuffer);
        mEditorGameFramebuffer = 0;
    }

    mEditorGameRenderWidth = 0;
    mEditorGameRenderHeight = 0;
}

bool Game::EnsureUGCPreviewRenderTarget()
{
    const int previewWidth = mRequestedUGCPreviewRenderWidth;
    const int previewHeight = mRequestedUGCPreviewRenderHeight;
    if (previewWidth <= 0 || previewHeight <= 0) {
        return false;
    }

    if (mUGCPreviewFramebuffer == 0) {
        glGenFramebuffers(1, &mUGCPreviewFramebuffer);
        glGenTextures(1, &mUGCPreviewTexture);
        glGenRenderbuffers(1, &mUGCPreviewDepthBuffer);
    }
    if (mUGCPreviewRenderWidth == previewWidth &&
        mUGCPreviewRenderHeight == previewHeight) {
        return true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, mUGCPreviewFramebuffer);
    glBindTexture(GL_TEXTURE_2D, mUGCPreviewTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, previewWidth, previewHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, mUGCPreviewTexture, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, mUGCPreviewDepthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8,
                          previewWidth, previewHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, mUGCPreviewDepthBuffer);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
        GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (!complete) {
        DestroyUGCPreviewRenderTarget();
        return false;
    }
    mUGCPreviewRenderWidth = previewWidth;
    mUGCPreviewRenderHeight = previewHeight;
    return true;
}

void Game::SetUGCPreviewRenderSize(int width, int height)
{
    constexpr int minimumPreviewWidth = 320;
    constexpr int minimumPreviewHeight = 180;
    constexpr int maximumPreviewWidth = 2048;
    constexpr int maximumPreviewHeight = 1152;
    mRequestedUGCPreviewRenderWidth = std::clamp(
        width, minimumPreviewWidth, maximumPreviewWidth);
    mRequestedUGCPreviewRenderHeight = std::clamp(
        height, minimumPreviewHeight, maximumPreviewHeight);
}

void Game::AdjustUGCPreviewYaw(float yawDeltaRadians)
{
    constexpr float fullTurnRadians = 6.28318530718f;
    mUGCPreviewYawRadians += yawDeltaRadians;
    while (mUGCPreviewYawRadians > 3.14159265359f) {
        mUGCPreviewYawRadians -= fullTurnRadians;
    }
    while (mUGCPreviewYawRadians < -3.14159265359f) {
        mUGCPreviewYawRadians += fullTurnRadians;
    }
}

void Game::DestroyUGCPreviewRenderTarget()
{
    if (mUGCPreviewDepthBuffer) glDeleteRenderbuffers(1, &mUGCPreviewDepthBuffer);
    if (mUGCPreviewTexture) glDeleteTextures(1, &mUGCPreviewTexture);
    if (mUGCPreviewFramebuffer) glDeleteFramebuffers(1, &mUGCPreviewFramebuffer);
    mUGCPreviewDepthBuffer = 0;
    mUGCPreviewTexture = 0;
    mUGCPreviewFramebuffer = 0;
    mUGCPreviewRenderWidth = 0;
    mUGCPreviewRenderHeight = 0;
}

void Game::DrawUGCPreviewFrame()
{
    const int previewWidth = mUGCPreviewRenderWidth;
    const int previewHeight = mUGCPreviewRenderHeight;
    if (previewWidth <= 0 || previewHeight <= 0) {
        return;
    }
    glViewport(0, 0, previewWidth, previewHeight);
    glClearColor(0.025f, 0.035f, 0.075f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!mRenderer3D || !mCameraSystem) {
        return;
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mUIRenderer->DrawSkyBox(previewWidth, previewHeight);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    const CameraPose editorPose = mCameraSystem->GetDebugCameraPose();
    const float targetPreviewY =
        static_cast<float>(mUGCPreviewEditLayer) * mUGCGridSize;
    if (!mHasUGCPreviewFocusY) {
        mUGCPreviewFocusY = targetPreviewY;
        mHasUGCPreviewFocusY = true;
    } else {
        const float smoothing = 1.0f - std::exp(
            -10.0f * std::max(0.0f, mLastDeltaTime));
        mUGCPreviewFocusY +=
            (targetPreviewY - mUGCPreviewFocusY) * smoothing;
    }
    glm::vec3 previewTarget = editorPose.target;
    previewTarget.y = mUGCPreviewFocusY;
    const glm::vec3 basePreviewDirection =
        glm::normalize(glm::vec3(1.0f, 0.75f, 1.0f));
    const float previewYawCosine = std::cos(mUGCPreviewYawRadians);
    const float previewYawSine = std::sin(mUGCPreviewYawRadians);
    const glm::vec3 previewDirection = glm::normalize(glm::vec3(
        basePreviewDirection.x * previewYawCosine +
            basePreviewDirection.z * previewYawSine,
        mIsUGCPreviewViewedFromBelow
            ? -basePreviewDirection.y
            : basePreviewDirection.y,
        -basePreviewDirection.x * previewYawSine +
            basePreviewDirection.z * previewYawCosine));
    constexpr float previewFieldOfViewDegrees = 55.0f;
    const float editorViewDistance = mUGCSessionState.IsOrthographicView()
        ? mUGCOrthographicHalfHeight /
              std::tan(glm::radians(previewFieldOfViewDegrees) * 0.5f)
        : glm::length(editorPose.position - editorPose.target);



    const float previewDistance = glm::clamp(
        editorViewDistance * 0.45f, 3.0f, 100.0f);
    const glm::vec3 previewPosition =
        previewTarget + previewDirection * previewDistance;
    const glm::vec3 previewUp = mIsUGCPreviewViewedFromBelow
        ? glm::vec3(0.0f, -1.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(
        previewPosition, previewTarget, previewUp);
    const glm::mat4 projection = glm::perspective(
        glm::radians(previewFieldOfViewDegrees),
        static_cast<float>(previewWidth) / previewHeight,
        0.1f,
        1000.0f);
    mRenderer3D->DrawScene(
        view,
        projection,
        previewPosition,
        UGCSceneLayerRenderMode::HighlightEditingLayerWithoutDimming,
        mUGCPreviewEditLayer);
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
    mUGCPlacementModelPreviewPositions.clear();
    if (!position) {
        mUGCPlacementModelPreviewActor.reset();
        return;
    }
    if (modelPath.empty()) {
        mUGCPlacementModelPreviewActor.reset();
        return;
    }

    if (!mUGCPlacementModelPreviewActor ||
        mUGCPlacementModelPreviewActor->GetModelPath() != modelPath) {
        mUGCPlacementModelPreviewActor = std::make_unique<Actor>(this);
        mUGCPlacementModelPreviewActor->SetModelPath(modelPath);
        if (mMeshLoadSystem) {
            mMeshLoadSystem->SetActorMesh(
                mUGCPlacementModelPreviewActor.get());
        }
    }

    mUGCPlacementModelPreviewActor->SetPos(*position);
    mUGCPlacementModelPreviewActor->SetScale(scale);
    mUGCPlacementModelPreviewActor->SetTextureOverridePath(
        textureOverridePath);
    mUGCPlacementModelPreviewActor->SetIsActive(true);
}

void Game::SetUGCPlacementModelPreviewPositions(
    const std::vector<glm::vec3>& positions,
    const std::string& modelPath,
    const glm::vec3& scale,
    const std::string& textureOverridePath)
{
    if (positions.empty()) {
        SetUGCPlacementModelPreview(std::nullopt);
        return;
    }

    SetUGCPlacementModelPreview(
        positions.front(), modelPath, scale, textureOverridePath);
    mUGCPlacementModelPreviewPositions = positions;
}

void Game::RemoveActor(Actor* actor)
{
    mWorld->RemoveActor(actor);
}

void Game::RemoveAllActor()
{
    if (mEnemyJewelDropSystem) {
        mEnemyJewelDropSystem->ClearRuntimeDrops();
    }
    mWorld->RemoveAllActors();
    mIsPlayerSplit = false;
    mControlledPlayerIndex = 0;
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

    const bool isSoloClone =
        !mIsPlayer2Joined && !mIsPlayerSplit &&
        !mWorld->GetPlayers().empty();
    if (isSoloClone) {
        player->SetIsActive(false);
        player->SetControlLocked(true);
    }



    if (mIsPlayer2Joined) {
        player->SetSplitForm(true);
        player->SetControlLocked(false);
        player->SetIsActive(true);
    }

    mWorld->AddPlayer(player);
}

void Game::RemoveAllPlayer()
{
    mWorld->RemoveAllPlayers();
    mIsPlayerSplit = false;
    mControlledPlayerIndex = 0;
}

bool Game::TogglePlayerSplit()
{
    if (!CanTogglePlayerSplit()) {
        return false;
    }

    const bool didChangeSplitState =
        mIsPlayerSplit ? MergePlayer() : SplitPlayer();
    if (didChangeSplitState && mSceneSystem) {
        mSceneSystem->OnPlayerSplitMergeSucceeded();
    }
    return didChangeSplitState;
}

bool Game::CanTogglePlayerSplit() const
{
    const bool allowsPlayerSplitToggle =
        mSceneSystem &&
        (mSceneSystem->IsPlaying() ||
         mSceneSystem->IsWaitingForTutorialPlayerSplitMerge());
    if (!allowsPlayerSplitToggle || !CanChangeSoloPlayerConfiguration()) {
        return false;
    }

    return !mIsPlayerSplit || AreSplitPlayersCloseEnoughToMerge();
}

bool Game::CanChangeSoloPlayerConfiguration() const
{
    const bool isWaitingForTutorialConfigurationAction =
        mSceneSystem &&
        (mSceneSystem->IsWaitingForTutorialPlayerSwitch() ||
         mSceneSystem->IsWaitingForTutorialPlayerSplitMerge());
    const bool allowsPlayerConfigurationScene =
        mSceneSystem &&
        (mSceneSystem->IsPlaying() ||
         isWaitingForTutorialConfigurationAction);
    const bool allowsNormalPlayerInput =
        mCameraSystem &&
        mCameraSystem->AllowsPlayerInput() &&
        (!mSequenceSystem ||
         !mSequenceSystem->LocksPlayerControl());
    const bool allowsPlayerConfigurationInput =
        isWaitingForTutorialConfigurationAction ||
        allowsNormalPlayerInput;
    return !mIsPlayer2Joined &&
           GetPlayers().size() >= 2 &&
           allowsPlayerConfigurationScene &&
           !GetIsPauseMenuOpen() &&
           allowsPlayerConfigurationInput;
}

bool Game::SplitPlayer()
{
    const std::vector<Player*>& players = GetPlayers();
    Player* mainPlayer = players[0];
    Player* splitPlayer = players[1];
    if (!mainPlayer || !splitPlayer) {
        return false;
    }

    mainPlayer->SetSplitForm(true);
    splitPlayer->SetSplitForm(true);

    splitPlayer->SetCurrentPlanet(mainPlayer->GetCurrentPlanet());
    splitPlayer->SetCurrentPlanetNum(mainPlayer->GetCurrentPlanetNum());
    splitPlayer->SetSphericalPlacement(
        mainPlayer->GetTheta(),
        mainPlayer->GetPhi(),
        mainPlayer->GetHeight());
    splitPlayer->SetOrientation(mainPlayer->GetOrientation());
    splitPlayer->SetFacingForwardVec(
        mainPlayer->GetFacingForwardVec());
    splitPlayer->SetCameraForwardDirection(
        -mainPlayer->GetFacingForwardVec(),
        mainPlayer->GetUpVec());
    splitPlayer->SetCameraYaw(mainPlayer->GetCameraYaw());
    splitPlayer->SetPos(mainPlayer->GetPos());
    splitPlayer->SetVelocity(mainPlayer->GetVelocity());
    splitPlayer->SetOnGround(mainPlayer->GetOnGround());
    splitPlayer->SetShouldJudgeLanding(!mainPlayer->GetOnGround());
    splitPlayer->RefreshFallbackUpVec();
    splitPlayer->SetControlLocked(false);
    splitPlayer->SetIsActive(true);

    mIsPlayerSplit = true;
    SynchronizeSoloSplitResources(*mainPlayer);
    SelectControlledPlayer(1);
    return true;
}

bool Game::MergePlayer()
{
    if (!AreSplitPlayersCloseEnoughToMerge()) {
        return false;
    }

    return MergePlayerInto(mControlledPlayerIndex);
}

bool Game::AreSplitPlayersCloseEnoughToMerge() const
{
    const std::vector<Player*>& players = GetPlayers();
    if (players.size() < 2) {
        return false;
    }

    const Player* mainPlayer = players[0];
    const Player* splitPlayer = players[1];
    if (!mainPlayer || !splitPlayer ||
        !mainPlayer->GetIsActive() ||
        !splitPlayer->GetIsActive()) {
        return false;
    }

    const float playerDistance =
        glm::length(mainPlayer->GetPos() - splitPlayer->GetPos());
    return playerDistance <= playerMergeMaximumDistanceWorldUnits;
}

bool Game::MergePlayerInto(int targetPlayerIndex)
{
    const std::vector<Player*>& players = GetPlayers();
    if (players.size() < 2 ||
        targetPlayerIndex < 0 ||
        targetPlayerIndex >= 2) {
        return false;
    }

    Player* mainPlayer = players[0];
    Player* splitPlayer = players[1];
    if (!mainPlayer || !splitPlayer) {
        return false;
    }

    if (targetPlayerIndex == 1) {
        mainPlayer->SetCurrentPlanet(
            splitPlayer->GetCurrentPlanet());
        mainPlayer->SetCurrentPlanetNum(
            splitPlayer->GetCurrentPlanetNum());
        mainPlayer->SetSphericalPlacement(
            splitPlayer->GetTheta(),
            splitPlayer->GetPhi(),
            splitPlayer->GetHeight());
        mainPlayer->SetOrientation(
            splitPlayer->GetOrientation());
        mainPlayer->SetFacingForwardVec(
            splitPlayer->GetFacingForwardVec());
        mainPlayer->SetCameraForwardDirection(
            -splitPlayer->GetFacingForwardVec(),
            splitPlayer->GetUpVec());
        mainPlayer->SetCameraYaw(splitPlayer->GetCameraYaw());
        mainPlayer->SetPos(splitPlayer->GetPos());
        mainPlayer->SetVelocity(splitPlayer->GetVelocity());
        mainPlayer->SetOnGround(splitPlayer->GetOnGround());
        mainPlayer->SetShouldJudgeLanding(true);
        mainPlayer->RefreshFallbackUpVec();
    }

    mainPlayer->SetSplitForm(false);
    splitPlayer->SetSplitForm(false);
    splitPlayer->SetVelocity(glm::vec3(0.0f));
    splitPlayer->SetControlLocked(true);
    splitPlayer->SetIsActive(false);

    mIsPlayerSplit = false;
    SelectControlledPlayer(0);
    return true;
}

void Game::SelectControlledPlayer(int playerIndex)
{
    if (playerIndex == mControlledPlayerIndex) {
        return;
    }

    const int previousPlayerIndex = mControlledPlayerIndex;
    mControlledPlayerIndex = playerIndex;
    if (mCameraSystem) {
        mCameraSystem->SnapToControlledPlayer(
            previousPlayerIndex,
            playerIndex);
    }
}

bool Game::SwitchControlledPlayer()
{
    if (!CanSwitchControlledPlayer()) {
        return false;
    }

    const std::vector<Player*>& players = GetPlayers();

    const int previousIndex = mControlledPlayerIndex;
    int nextIndex = previousIndex;
    for (int offset = 1; offset <= static_cast<int>(players.size()); ++offset) {
        const int candidate =
            (previousIndex + offset) % static_cast<int>(players.size());
        Player* candidatePlayer =
            players[static_cast<std::size_t>(candidate)];
        if (candidatePlayer && candidatePlayer->GetIsActive()) {
            nextIndex = candidate;
            break;
        }
    }

    if (nextIndex == previousIndex) {
        for (int candidateIndex = 0;
             candidateIndex < static_cast<int>(players.size());
             ++candidateIndex) {
            if (candidateIndex == previousIndex) {
                continue;
            }

            Player* waitingPlayer =
                players[static_cast<std::size_t>(candidateIndex)];
            if (!waitingPlayer || !waitingPlayer->CancelWaitingBoatRide()) {
                continue;
            }

            mPendingSoloSplitControlSwitchTimer = -1.0f;
            SelectControlledPlayer(candidateIndex);
            if (mSceneSystem) {
                mSceneSystem->OnPlayerSwitchSucceeded();
            }
            return true;
        }

        return false;
    }

    SelectControlledPlayer(nextIndex);
    if (mSceneSystem) {
        mSceneSystem->OnPlayerSwitchSucceeded();
    }
    return true;
}

bool Game::CanSwitchControlledPlayer() const
{
    const bool allowsPlayerSwitch =
        mSceneSystem &&
        (mSceneSystem->IsPlaying() ||
         mSceneSystem->IsWaitingForTutorialPlayerSwitch());
    if (!mIsPlayerSplit || !CanChangeSoloPlayerConfiguration() ||
        GetPlayers().size() < 2 || !allowsPlayerSwitch) {
        return false;
    }



    const std::vector<Player*>& players = GetPlayers();
    for (int index = 0; index < static_cast<int>(players.size()); ++index) {
        if (index != mControlledPlayerIndex && players[index] &&
            (players[index]->GetIsActive() ||
             players[index]->IsWaitingForBoat())) {
            return true;
        }
    }
    return false;
}

void Game::RequestSoloSplitControlSwitchAfterBoarding()
{
    if (!mIsPlayer2Joined && mIsPlayerSplit) {
        mPendingSoloSplitControlSwitchTimer = 0.5f;
    }
}

void Game::UpdatePendingSoloSplitControlSwitch(float deltaTime)
{
    if (mPendingSoloSplitControlSwitchTimer < 0.0f) {
        return;
    }

    mPendingSoloSplitControlSwitchTimer -= deltaTime;
    if (mPendingSoloSplitControlSwitchTimer > 0.0f) {
        return;
    }
    mPendingSoloSplitControlSwitchTimer = -1.0f;

    if (mIsPlayer2Joined || !mIsPlayerSplit) {
        return;
    }

    const std::vector<Player*>& players = GetPlayers();
    if (mControlledPlayerIndex >= 0 &&
        mControlledPlayerIndex < static_cast<int>(players.size()) &&
        players[static_cast<std::size_t>(mControlledPlayerIndex)] &&
        players[static_cast<std::size_t>(mControlledPlayerIndex)]->GetIsActive()) {
        return;
    }

    for (int index = 0; index < static_cast<int>(players.size()); ++index) {
        if (players[static_cast<std::size_t>(index)] &&
            players[static_cast<std::size_t>(index)]->GetIsActive()) {
            SelectControlledPlayer(index);
            return;
        }
    }
}

void Game::LoadData(bool isLoadPlayer)
{
    mStageFlowController->LoadData(*this, isLoadPlayer);
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

bool Game::StartUGCMode()
{
    return StartUGCModeWithStage(
        UserDataPaths::ResolveUGCWorkingStageFile().string(),
        false);
}

bool Game::StartUGCEditorTutorial()
{
    const std::filesystem::path tutorialTemplatePath =
        "../assets/data/stage/ugc_tutorial_template.yaml";
    const std::filesystem::path tutorialStagePath =
        UserDataPaths::ResolveUGCTutorialStageFile();
    std::error_code copyError;
    std::filesystem::copy_file(
        tutorialTemplatePath,
        tutorialStagePath,
        std::filesystem::copy_options::overwrite_existing,
        copyError);
    if (copyError) {
        std::cerr << "Failed to reset UGC editor tutorial: "
                  << copyError.message() << '\n';
        return false;
    }
    return StartUGCModeWithStage(tutorialStagePath.string(), true);
}

bool Game::StartUGCModeWithStage(
    const std::string& yamlPath,
    bool isTutorial)
{
    constexpr int UGCStageNumber = 0;

    if (!LoadDebugStage(UGCStageNumber, yamlPath)) {
        return false;
    }

    mIsUGCEditorTutorialActive = isTutorial;
    ClosePauseMenu();
    mUGCSessionState.EnterEditor();
    mUGCOrthographicHalfHeight = 20.0f;
    mIsDebugEditorShowing = true;
    SetFreeCameraMode(true);




    mSceneSystem->StartPlayingScene();

    if (mCameraSystem) {
        CameraPose pose;
        pose.position = glm::vec3(0.0f, 30.0f, 0.0f);
        pose.target = glm::vec3(0.0f);
        pose.up = glm::vec3(0.0f, 0.0f, -1.0f);
        pose.fieldOfViewDegrees = 55.0f;
        mCameraSystem->SetDebugCameraPose(pose);
    }
    return true;
}

bool Game::HasSeenUGCEditorTutorial() const
{
    if (!mStageProgressSystem) {
        return false;
    }
    return mStageProgressSystem->HasShownConversation(
               "tutorial:ugc_editor_completed") ||
        mStageProgressSystem->HasShownConversation(
               "tutorial:ugc_editor_skipped");
}

bool Game::FinishUGCEditorTutorial(bool wasCompleted)
{
    if (mStageProgressSystem) {
        mStageProgressSystem->MarkConversationShown(
            wasCompleted
                ? "tutorial:ugc_editor_completed"
                : "tutorial:ugc_editor_skipped");
    }
    mIsUGCEditorTutorialActive = false;
    return StartUGCMode();
}

void Game::OpenUGCWorkBrowser()
{
    if (!mSceneSystem || !mSceneSystem->IsTitle()) {
        return;
    }
    mUGCSessionState.OpenWorkBrowser();
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
            if (HasSeenUGCEditorTutorial()) {
                StartUGCMode();
            } else {
                StartUGCEditorTutorial();
            }
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
    mUGCSessionState.CloseWorkBrowser();
}

void Game::StartUGCPlaytest()
{
    if (!mUGCSessionState.StartPlaytest()) {
        return;
    }
    mIsDebugEditorShowing = false;
    SetFreeCameraMode(false);
    ReloadCurrentStage();
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
    if (!mUGCSessionState.StartVerification(workFileName)) {
        return;
    }
    StartUGCPlaytest();
}

void Game::ReturnToUGCEditor()
{
    if (!mUGCSessionState.ReturnToEditor()) {
        return;
    }
    ReloadCurrentStage();
    mIsDebugEditorShowing = true;
    SetFreeCameraMode(true);
    if (mIsUGCEditorTutorialActive && mUIRenderer) {
        mUIRenderer->NotifyUGCEditorTutorialReturnedFromPlaytest();
    }
}

void Game::ExitUGCMode()
{
    if (!mSceneSystem) {
        return;
    }
    mSceneSystem->RequestFadeAction(
        [this]() { CompleteUGCModeExit(false); });
}

void Game::CompleteUGCModeExit(bool shouldOpenWorkBrowser)
{
    mIsUGCClearTransitionInProgress = false;
    mPendingUGCClearTransitionWorkFileName.reset();
    mUGCSessionState.Exit();
    mIsUGCEditorTutorialActive = false;
    mIsDebugEditorShowing = false;
    SetFreeCameraMode(false);

    constexpr int InitialStageNumber = 0;
    constexpr const char* InitialStageYamlPath =
        "../assets/data/stage/house.yaml";
    if (LoadDebugStage(InitialStageNumber, InitialStageYamlPath)) {
        mSceneSystem->EnterTitleAtFadeMidpoint();
        mAudioSystem->TryChangeBGM();
        if (shouldOpenWorkBrowser) {
            OpenUGCWorkBrowser();
        }
    }
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

void Game::CreatePlayer2()
{
    if (mIsPlayer2Joined) {
        return;
    }

    if (!IsGameControllerConnected()) {
        return;
    }

    if (GetPlayers().size() < 2) {
        const bool created = mActorLoadSystem->CreatePlayerFromCurrentStage(2);
        if (!created) {
            return;
        }
    }



    if (!SplitPlayer()) {
        return;
    }
    mIsPlayer2Joined = true;
    SelectControlledPlayer(0);
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




    if (!mIsPlayer2Joined && mIsPlayerSplit) {
        MergePlayerInto(mControlledPlayerIndex);
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
    if (mUGCSessionState.IsModeActive()) {
        if (mIsUGCClearTransitionInProgress) {
            return;
        }
        // Actor走査中にステージを再読込すると走査中のActorを破棄するため、保存と遷移を次フレームへ遅延する。



        mUGCSessionState.MarkClearCompletionPending();
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
    if (!mPendingUGCClearTransitionWorkFileName) {
        mPendingUGCClearTransitionWorkFileName =
            mUGCSessionState.ConsumeClearCompletion();
    }
    if (!mPendingUGCClearTransitionWorkFileName || !mSceneSystem) {
        return;
    }

    const std::string completedWorkFileName =
        *mPendingUGCClearTransitionWorkFileName;
    if (completedWorkFileName.empty()) {
        if (mSceneSystem->RequestFadeAction(
                [this]() {
                    ReturnToUGCEditor();
                    mIsUGCClearTransitionInProgress = false;
                })) {
            mPendingUGCClearTransitionWorkFileName.reset();
            mIsUGCClearTransitionInProgress = true;
        }
        return;
    }

    const auto completeVerificationAndReturnToBrowser =
        [this, completedWorkFileName]() {
            if (mUIRenderer) {
                mUIRenderer->CompleteUGCVerification(
                    completedWorkFileName);
            }
            CompleteUGCModeExit(true);
        };
    if (mSceneSystem->RequestFadeAction(
            completeVerificationAndReturnToBrowser)) {
        mPendingUGCClearTransitionWorkFileName.reset();
        mIsUGCClearTransitionInProgress = true;
    }
}

bool Game::IsStageCleared(int stageNum) const
{
    return mStageProgressSystem &&
           mStageProgressSystem->IsStageCleared(stageNum);
}

void Game::MarkStageCleared(int stageNum)
{
    SetStageCleared(stageNum, true);
}

void Game::SetStageCleared(int stageNum, bool isCleared)
{
    if (!mStageProgressSystem) {
        return;
    }

    const bool changed =
        mStageProgressSystem->SetStageCleared(stageNum, isCleared);
    if (changed && mWorld) {
        mWorld->RefreshActorProgressVisibility();
    }
    if (changed && mPhysicsSystem) {
        mPhysicsSystem->Initialize();
    }
}

bool Game::HasCompletedTutorial(const std::string& tutorialId) const
{
    const std::string completedTutorialId =
        BuildCompletedTutorialId(tutorialId);
    return mStageProgressSystem &&
           !completedTutorialId.empty() &&
           mStageProgressSystem->HasShownConversation(
               completedTutorialId);
}

void Game::MarkTutorialCompleted(const std::string& tutorialId)
{
    if (!mStageProgressSystem) {
        return;
    }

    const std::string completedTutorialId =
        BuildCompletedTutorialId(tutorialId);
    if (completedTutorialId.empty()) {
        return;
    }

    mStageProgressSystem->MarkConversationShown(
        completedTutorialId);
}

bool Game::HasShownNPCConversation(const NPC* npc) const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasShownConversation(
               BuildNPCConversationId(npc));
}

void Game::MarkNPCConversationShown(const NPC* npc)
{
    if (!mStageProgressSystem) {
        return;
    }

    mStageProgressSystem->MarkConversationShown(
        BuildNPCConversationId(npc));
}

bool Game::HasSeenBaseIntro() const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasShownConversation(
               "cinematic:base_intro");
}

void Game::MarkBaseIntroSeen()
{
    if (mStageProgressSystem) {
        mStageProgressSystem->MarkConversationShown(
            "cinematic:base_intro");
    }
}

bool Game::HasCompletedNPCOpeningTrigger(
    const NPC* npc, std::size_t talkPageIndex) const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasShownConversation(
               BuildNPCOpeningTriggerId(npc, talkPageIndex));
}

void Game::MarkNPCOpeningTriggerCompleted(
    const NPC* npc, std::size_t talkPageIndex)
{
    if (mStageProgressSystem) {
        mStageProgressSystem->MarkConversationShown(
            BuildNPCOpeningTriggerId(npc, talkPageIndex));
    }
}

bool Game::AreAllMainStagesCleared() const
{
    constexpr int firstMainStage = 1;
    constexpr int lastMainStage = 5;
    for (int stageNum = firstMainStage; stageNum <= lastMainStage; ++stageNum) {
        if (!IsStageCleared(stageNum)) {
            return false;
        }
    }
    return true;
}

bool Game::HasCompletedEndingRoll() const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasCompletedEndingRoll();
}

void Game::MarkEndingRollCompleted()
{
    if (mStageProgressSystem) {
        mStageProgressSystem->SetEndingRollCompleted();
    }
}

bool Game::HasCompletedNPCEndingTrigger(
    const NPC* npc, std::size_t talkPageIndex) const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasShownConversation(
               BuildNPCEndingTriggerId(npc, talkPageIndex));
}

void Game::MarkNPCEndingTriggerCompleted(
    const NPC* npc, std::size_t talkPageIndex)
{
    if (mStageProgressSystem) {
        mStageProgressSystem->MarkConversationShown(
            BuildNPCEndingTriggerId(npc, talkPageIndex));
    }
}

std::string Game::BuildNPCConversationId(const NPC* npc) const
{
    if (!npc || npc->GetStageYamlIndex() < 0) {
        return {};
    }

    return GetCurrentStageYamlPath() + "|" +
           npc->GetStageSequenceName() + ":" +
           std::to_string(npc->GetStageYamlIndex()) +
           "|clear:" +
           std::to_string(
               npc->ResolveTalkStageClearCondition());
}

std::string Game::BuildNPCOpeningTriggerId(
    const NPC* npc, std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty()
               ? std::string()
               : conversationId + "|opening:" +
                     std::to_string(talkPageIndex);
}

std::string Game::BuildNPCEndingTriggerId(
    const NPC* npc, std::size_t talkPageIndex) const
{
    const std::string conversationId = BuildNPCConversationId(npc);
    return conversationId.empty()
               ? std::string()
               : conversationId + "|ending:" +
                     std::to_string(talkPageIndex);
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



    if (mStageProgressSystem) {
        mStageProgressSystem->Save();
    }
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void Game::RestartGame()
{



    if (!mIsPlayer2Joined && mIsPlayerSplit) {
        MergePlayerInto(0);
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
    if (mIsPlayer2Joined || !mIsPlayerSplit) {
        return;
    }

    const std::vector<Player*>& players = GetPlayers();
    if (players.size() < 2 ||
        (players[0] != &sourcePlayer && players[1] != &sourcePlayer)) {
        return;
    }




    for (Player* player : players) {
        if (!player || player == &sourcePlayer) {
            continue;
        }

        if (player->GetMaxHp() != sourcePlayer.GetMaxHp()) {
            player->SetMaxHp(sourcePlayer.GetMaxHp());
        }
        if (player->GetHp() != sourcePlayer.GetHp()) {
            player->SetHp(sourcePlayer.GetHp());
        }
        if (player->GetJewelCount() != sourcePlayer.GetJewelCount()) {
            player->SetJewelCount(sourcePlayer.GetJewelCount());
        }
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
    if (!mIsPlayer2Joined) {
        if (Player* controlledPlayer = GetControlledPlayer()) {
            return controlledPlayer;
        }
    }

    return mWorld->GetMainPlayer();
}

Player* Game::GetControlledPlayer() const
{
    const std::vector<Player*>& players = GetPlayers();
    if (mControlledPlayerIndex < 0 ||
        mControlledPlayerIndex >= static_cast<int>(players.size())) {
        return players.empty() ? nullptr : players[0];
    }

    return players[static_cast<std::size_t>(mControlledPlayerIndex)];
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
    return BuildNPCConversationId(npc);
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
            if (npc && BuildNPCConversationId(npc) == conversationId) {
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
