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
#include "system/InputSystem.h"
#include "system/MeshLoadSystem.h"
#include "system/PhysicsSystem.h"
#include "system/SceneSystem.h"
#include "system/UILoadSystem.h"

#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"

#include "utils/MathUtils.h"

#include <algorithm>
#include <iostream>
#include <limits>

Game::Game()
    : mWindow(nullptr),
      mSdlController(nullptr),
      mCurrentStage(nullptr),
      mCurrentStageNum(0),
      mHitStopTimer(-1.0f),
      mLastTime(0.0),
      mIsPlayer2Joined(false),
      mCurrentStageYamlPath("../assets/data/stage/house.yaml"),
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

    InitializeGameController();
    CreateGameSystems();

    if (isDebugMode) {
        mIsDebugMode = true;
        mCurrentStageNum = 1;
        mCurrentStageYamlPath = "../assets/data/stage/stage2.yaml";
        mSceneSystem->StartPlayingScene();
    }

    constexpr int stageCount = 5;
    CreateStages(stageCount);

    ReloadCurrentStage();

    mLastTime = glfwGetTime();
    glEnable(GL_DEPTH_TEST);

    return true;
}

bool Game::InitializeGLFW()
{
    if (!glfwInit()) {
        // std::cerr << "Failed to init GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //     GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    //     const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    //     mWindow = glfwCreateWindow(mode->width, mode->height, "Engine",
    //     monitor, nullptr);

    mWindow = glfwCreateWindow(800, 450, "Slime'sSpaceTravel", nullptr, nullptr);
    if (!mWindow) {
        // std::cerr << "Failed to create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(mWindow);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        // std::cerr << "Failed to init GLEW" << std::endl;
        glfwDestroyWindow(mWindow);
        glfwTerminate();
        return false;
    }

    return true;
}

void Game::InitializeGameController()
{
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) == 0) {
        CheckGameControllerConnected();
    }
}

void Game::CreateGameSystems()
{
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
}

void Game::CreateStages(int stageCount)
{
    for (int i = 0; i < stageCount; i++) {
        auto stageUnique = std::make_unique<Stage>();
        Stage* stage = stageUnique.get();

        mStagesUnique.emplace_back(std::move(stageUnique));
        mStages.emplace_back(stage);

        if (i == 0) {
            mCurrentStage = stage;
        }
    }
}

void Game::ReloadCurrentStage()
{
    LoadData(true);
    mPhysicsSystem->Initialize();
    mAudioSystem->TryChangeBGM();
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
    if (mSdlController) {
        SDL_GameControllerClose(mSdlController);
        mSdlController = nullptr;
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

    if (mIsPauseMenuOpen) {
        return;
    }

    ProcessActorsInput();
    mCameraSystem->ProcessInput();
}

void Game::MovePauseMenuSelection(int delta)
{
    constexpr int menuItemCount = 4;
    mPauseMenuSelectedIndex = (mPauseMenuSelectedIndex + delta + menuItemCount) % menuItemCount;
}

void Game::TogglePauseMenu()
{
    mIsPauseMenuOpen = !mIsPauseMenuOpen;

    if (mIsPauseMenuOpen) {
        mPauseMenuSelectedIndex = 0;
    }
}

void Game::ClosePauseMenu()
{
    mIsPauseMenuOpen = false;
}

void Game::ExecutePauseMenuItem()
{
    switch (mPauseMenuSelectedIndex) {
    case 0:
        ClosePauseMenu();
        break;

    case 1:
        ReturnToBase();
        break;

    case 2:
        OpenFeedbackForm();
        break;

    case 3:
        FinishGame();
        break;

    default:
        break;
    }
}

void Game::ReturnToBase()
{
    ClosePauseMenu();

    if (IsInBase()) {
        return;
    }

    mSceneSystem->RequestStageChange(0);
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
}

void Game::ToggleFreeCameraMode()
{
    mIsFreeCameraMode = !mIsFreeCameraMode;
}

void Game::ProcessActorsInput()
{
    if (!mSceneSystem->IsPlaying()) {
        return;
    }

    for (const auto& actorUnique : mActors) {
        actorUnique->ProcessInput();
    }
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

    if (mIsPauseMenuOpen) {
        return;
    }

    if (mIsFreeCameraMode) {
        mCameraSystem->Update(deltaTime);
        return;
    }

    mSceneSystem->Update(deltaTime);

    if (mSceneSystem->CanUpdateWorld()) {
        UpdateActors(deltaTime);
        mCameraSystem->Update(deltaTime);
    }
}

void Game::UpdateActors(float deltaTime)
{
    for (const auto& actorUnique : mActors) {
        actorUnique->Update(deltaTime);
    }
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
    mActors.emplace_back(std::move(actor));
}

void Game::RemoveActor(Actor* actor)
{
    auto iter = std::find_if(mActors.begin(), mActors.end(),
                             [actor](const std::unique_ptr<Actor>& current) { return current.get() == actor; });

    if (iter != mActors.end()) {
        std::iter_swap(iter, mActors.end() - 1);
        mActors.pop_back();
    }
}

void Game::RemoveAllActor()
{
    mPlayers.clear();
    mActors.clear();
}

void Game::LoadData(bool isLoadPlayer)
{
    RemoveAllActor();
    mActorLoadSystem->LoadData(isLoadPlayer);
}

void Game::ChangeStage(int stageNum)
{
    if (stageNum < 0 || stageNum >= static_cast<int>(mStages.size())) {
        return;
    }

    mCurrentStage = mStages[stageNum];
    mCurrentStageNum = stageNum;
    mCurrentStageYamlPath = "../assets/data/stage/stage" + std::to_string(stageNum) + ".yaml";
}

void Game::CheckGameControllerConnected()
{
    if (mSdlController) {
        return;
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            mSdlController = SDL_GameControllerOpen(i);
            break;
        }
    }
}

void Game::CreatePlayer2()
{
    if (mIsPlayer2Joined) {
        return;
    }

    if (!IsGameControllerConnected()) {
        return;
    }

    const bool created = mActorLoadSystem->CreatePlayerFromCurrentStage(2);
    if (!created) {
        return;
    }

    mIsPlayer2Joined = true;
}

void Game::OnBoatStageChangeRequested(int destStage)
{
    if (mCurrentStageNum != 0) {
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
    mSceneSystem->OnStageClear();
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
    mPlayers[0]->GetCurrentPlanet()->OnBoatPartsObtained();
}

Player* Game::FindNearestPlayer(Actor* actor) const
{
    if (!actor) {
        return nullptr;
    }

    Player* nearestPlayer = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    for (Player* player : mPlayers) {
        if (!player) {
            continue;
        }

        const glm::vec3 toPlayer = player->GetPos() - actor->GetPos();
        const float dist = glm::dot(toPlayer, toPlayer);

        if (dist < nearestDist) {
            nearestDist = dist;
            nearestPlayer = player;
        }
    }

    return nearestPlayer;
}

void Game::FinishGame()
{
    glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
}

void Game::RestartGame()
{
    for (auto player : mPlayers) {
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

void Game::OnPlayerFinishCharging(int playerNum)
{
    mAudioSystem->PlaySE("air_charged_se");
    VibrateControllerForPlayer(playerNum, 0, 10000, 200);
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
    if (playerNum != 1) {
        return;
    }

    if (!mSdlController) {
        return;
    }

    SDL_GameControllerRumble(mSdlController, lowFrequency, highFrequency, duration);
}
