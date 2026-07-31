#include <GL/glew.h>

#include "Game.h"
#include "Stage.h"

#include "actor/Actor.h"
#include "actor/Enemy.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "state/GameProgressState.h"
#include "system/ActorLoadSystem.h"
#include "system/AudioSystem.h"
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
#include "system/sequence/SequenceSystem.h"

#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"

#include "utils/MathUtils.h"

#include <algorithm>
#include <iostream>

Game::Game()
    : mWindow(nullptr),
      mHitStopTimer(-1.0f),
      mLastTime(0.0),
      mIsPlayer2Joined(false),
      mIsDebugEditorShowing(false),
      mIsFreeCameraMode(false),
      mIsDebugMode(false)
{
}

Game::~Game() = default;

bool Game::Initialize(bool isDebugMode)
{
    if (!InitializeGLFW()) {
        return false;
    }

    CreateGameSystems();
    InitializeGameController();

    constexpr int stageCount = 5;
    CreateStages(stageCount);

    if (isDebugMode) {
        mIsDebugMode = true;
        mWorld->ChangeStage(1);
        mStageFlowController->SetCurrentStageYamlPath("../assets/data/stage/test.yaml");
        mSceneSystem->StartPlayingScene();
    }

    ReloadCurrentStage();

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
    mGamepadRumbleService = std::make_unique<GamepadRumbleService>();

    mAudioSystem = std::make_unique<AudioSystem>(this);
    mUIRenderer = std::make_unique<UIRenderer>(this);
    mRenderer3D = std::make_unique<Renderer3D>(this);
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

void Game::ReloadCurrentStage()
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

    mStageFlowController->ReloadCurrentStage(*this);
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
        glfwPollEvents();
        ProcessInput();
        UpdateGame();
        GenerateOutput();
    }
}

void Game::Shutdown()
{
    if (mGamepadRumbleService) {
        mGamepadRumbleService->Shutdown();
    }

    if (mAudioSystem) {
        mAudioSystem->Shutdown();
    }

    SDL_Quit();

    if (mWindow) {
        glfwDestroyWindow(mWindow);
        mWindow = nullptr;
    }

    glfwTerminate();
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
    mPauseMenuController->MoveSelection(delta);
}

void Game::TogglePauseMenu()
{
    mPauseMenuController->Toggle();
}

void Game::ClosePauseMenu()
{
    mPauseMenuController->Close();
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

void Game::ToggleDebugEditor()
{
    mIsDebugEditorShowing = !mIsDebugEditorShowing;
    if (mPhysicsSystem) {
        mPhysicsSystem->Initialize();
    }
}

void Game::ToggleFreeCameraMode()
{
    mIsFreeCameraMode = !mIsFreeCameraMode;
}

void Game::ProcessActorsInput()
{
    const bool isWaitingForTutorialPlayerJump =
        mSceneSystem->IsWaitingForTutorialPlayerJump();
    const bool allowsPlayerControl =
        (mSceneSystem->IsPlaying() ||
         isWaitingForTutorialPlayerJump) &&
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

void Game::UpdateGame()
{
    CheckGameControllerConnected();

    const double currentTime = glfwGetTime();
    const float deltaTime = std::min(0.04f, static_cast<float>(currentTime - mLastTime));
    mLastTime = currentTime;

    if (mHitStopTimer >= 0.0f) {
        mHitStopTimer -= deltaTime;
        return;
    }

    if (mPauseMenuController->IsOpen()) {
        return;
    }

    if (mIsFreeCameraMode) {
        if (mSequenceSystem) {
            mSequenceSystem->Update(deltaTime);
        }
        mCameraSystem->Update(deltaTime);
        return;
    }

    mSceneSystem->Update(deltaTime);

    bool cameraUpdated = false;
    const bool shouldUpdateEntireWorld =
        mSceneSystem->CanUpdateWorld();
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
}

void Game::GenerateOutput()
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mUIRenderer->DrawSkyBox();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    mRenderer3D->Draw();

    glDisable(GL_DEPTH_TEST);
    mUIRenderer->Draw();
    glEnable(GL_DEPTH_TEST);

    glfwSwapBuffers(mWindow);
}

void Game::AddActor(std::unique_ptr<Actor> actor)
{
    mWorld->AddActor(std::move(actor));
}

void Game::RemoveActor(Actor* actor)
{
    mWorld->RemoveActor(actor);
}

void Game::RemoveAllActor()
{
    mWorld->RemoveAllActors();
    mControlledPlayerIndex = 0;
}

void Game::AddPlayer(Player* player)
{
    mWorld->AddPlayer(player);
}

void Game::RemoveAllPlayer()
{
    mWorld->RemoveAllPlayers();
    mControlledPlayerIndex = 0;
}

bool Game::SwitchControlledPlayer()
{
    const std::vector<Player*>& players = GetPlayers();
    const bool allowsPlayerSwitch =
        mSceneSystem &&
        (mSceneSystem->IsPlaying() ||
         mSceneSystem->IsWaitingForTutorialPlayerSwitch());
    if (mIsPlayer2Joined || players.size() < 2 ||
        !allowsPlayerSwitch ||
        GetIsPauseMenuOpen() ||
        !mCameraSystem || !mCameraSystem->AllowsPlayerInput() ||
        (mSequenceSystem && mSequenceSystem->LocksPlayerControl())) {
        return false;
    }

    const int previousIndex = mControlledPlayerIndex;
    int nextIndex = previousIndex;
    for (int offset = 1; offset <= static_cast<int>(players.size()); ++offset) {
        const int candidate =
            (previousIndex + offset) % static_cast<int>(players.size());
        if (players[static_cast<std::size_t>(candidate)]) {
            nextIndex = candidate;
            break;
        }
    }

    if (nextIndex == previousIndex) {
        return false;
    }

    mCameraSystem->BeginPlayerSwitchTransition(previousIndex, nextIndex);
    mControlledPlayerIndex = nextIndex;
    return true;
}

void Game::LoadData(bool isLoadPlayer)
{
    mStageFlowController->LoadData(*this, isLoadPlayer);
}

void Game::ChangeStage(int stageNum)
{
    mStageFlowController->ChangeStage(*mWorld, stageNum);
}

bool Game::DebugChangeStage(int stageNum, const std::string& yamlPath)
{
    if (yamlPath.empty() || stageNum < 0 || stageNum >= static_cast<int>(GetStages().size())) {
        return false;
    }

    if (!mWorld->ChangeStage(stageNum)) {
        return false;
    }

    mStageFlowController->SetCurrentStageYamlPath(yamlPath);
    mSceneSystem->StartPlayingScene();
    ReloadCurrentStage();

    const bool isBaseStageYaml =
        yamlPath.ends_with("/stage0.yaml") || yamlPath.ends_with("\\stage0.yaml");
    if (isBaseStageYaml && mSequenceSystem) {
        if (mSequenceSystem->PlayCinematicChainThenSequence(
                {"base_sequence", "base_second_sequence"},
                "base_arrival_template")) {
            if (Player* player = GetMainPlayer()) {
                player->SetIsActive(false);
            }
        } else {
            mSequenceSystem->Play("base_arrival_template");
        }
    }

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

    mIsPlayer2Joined = true;
    mControlledPlayerIndex = 0;
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
    mAudioSystem->TryChangeBGM();
}

void Game::OnStarObtained()
{
    MarkStageCleared(GetCurrentStageNum());
    mSceneSystem->OnStageClear();
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
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void Game::RestartGame()
{
    for (auto player : GetPlayers()) {
        player->Restart();
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
    VibrateControllerForPlayer(playerNum, 25000, 0, 500);
}

void Game::VibrateControllerForPlayer(int playerNum, int lowFrequency, int highFrequency, int duration)
{
    mGamepadRumbleService->VibrateForPlayer(playerNum, lowFrequency, highFrequency, duration);
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
