#include "system/SceneSystem.h"

#include "Game.h"
#include "Stage.h"

#include "actor/Boat.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "system/AudioSystem.h"
#include "system/scene/SceneTransitionController.h"
#include "system/scene/TalkController.h"
#include "system/scene/TutorialController.h"

#include <SDL2/SDL_mixer.h>
#include <glm/glm.hpp>

SceneSystem::SceneSystem(Game* game)
    : mGame(game),
      mFadeTimer(-1.0f),
      mClearTimer(-1.0f),
      mIsFadeOut(false),
      mHasPendingStageChange(false),
      mNextStageNum(-1)
{
    mGameProgressState = std::make_unique<GameProgressState>(game);
    mUIState = std::make_unique<UIState>(game);

    CreateControllers();
}

SceneSystem::~SceneSystem() = default;

void SceneSystem::CreateControllers()
{
    mTransitionController = std::make_unique<SceneTransitionController>(
        mGame, mGameProgressState.get(), mUIState.get(), mFadeTimer, mIsFadeOut, mHasPendingStageChange, mNextStageNum);
    mTalkController =
        std::make_unique<TalkController>(mGame, mGameProgressState.get(), mUIState.get(), mTalkingNPC, mTalkingPlayer);
    mTutorialController = std::make_unique<TutorialController>(mGame, mGameProgressState.get(), mUIState.get());
}

void SceneSystem::Update(float deltaTime)
{
    mTransitionController->UpdateFade(deltaTime);
    mTutorialController->Update(deltaTime);
    mTalkController->Update(deltaTime);
    UpdateClearTimer(deltaTime);
}

void SceneSystem::OnConfirmPressed(int playerNum)
{
    if (mFadeTimer >= 0.0f) {
        return;
    }

    const auto sceneState = mGameProgressState->GetSceneState();

    switch (sceneState) {
    case GameProgressState::SceneState::Title:
        StartOpening();
        break;

    case GameProgressState::SceneState::Opening:
        mTalkController->TryAdvanceTalkFromConfirm();
        break;

    case GameProgressState::SceneState::Talking:
        if (mTutorialController->HasActiveTutorial()) {
            mTutorialController->TryAdvanceFromConfirm();
        } else {
            mTalkController->TryAdvanceTalkFromConfirm();
        }
        break;

    case GameProgressState::SceneState::Playing:
        mTalkController->TryStartTalkWithNPC(playerNum);
        break;

    case GameProgressState::SceneState::GameOver:
        RestartGame();
        break;

    default:
        break;
    }
}

bool SceneSystem::IsWaitingForTutorialPlayerAction() const
{
    return (mTutorialController &&
            mTutorialController->IsWaitingForPlayerAction()) ||
           (mTalkController &&
            mTalkController->IsWaitingForPlayerAction());
}

bool SceneSystem::IsWaitingForTutorialPlayerSwitch() const
{
    return (mTutorialController &&
            mTutorialController->IsWaitingForPlayerSwitch()) ||
           (mTalkController &&
            mTalkController->IsWaitingForPlayerSwitch());
}

bool SceneSystem::IsWaitingForTutorialPlayerJump() const
{
    return (mTutorialController &&
            mTutorialController->IsWaitingForPlayerJump()) ||
           (mTalkController &&
            mTalkController->IsWaitingForPlayerJump());
}

bool SceneSystem::IsWaitingForTutorialPlayerSplitMerge() const
{
    return mTutorialController &&
           mTutorialController->IsWaitingForPlayerSplitMerge();
}

void SceneSystem::OnPlayerSwitchSucceeded()
{
    if (mTutorialController) {
        mTutorialController->OnPlayerSwitchSucceeded();
    }
}

void SceneSystem::OnPlayerSplitMergeSucceeded()
{
    if (mTutorialController) {
        mTutorialController->OnPlayerSplitMergeSucceeded();
    }
}

bool SceneSystem::HasActiveTutorial() const
{
    return mTutorialController &&
           mTutorialController->HasActiveTutorial();
}

bool SceneSystem::IsTutorialActive(
    const std::string& tutorialId) const
{
    return mTutorialController &&
           mTutorialController->GetActiveTutorialId() ==
               tutorialId;
}

void SceneSystem::OnStartPressed()
{
    if (mGameProgressState->GetSceneState() == GameProgressState::SceneState::Opening && mFadeTimer <= -1.0f) {
        StartFadeIn();
        return;
    }

    if (!mGame->GetSdlController()) {
        return;
    }

    const bool operationUIShow = mUIState->GetIsOperationUIShow();
    mUIState->SetIsOperationUIShow(!operationUIShow);
}

void SceneSystem::StartOpening()
{
    mTransitionController->StartOpening();
}

void SceneSystem::RestartGame()
{
    mTransitionController->RequestFadeAction([this]() {
        mGame->RestartGame();
        StartPlayingScene();
    });
}

void SceneSystem::StartPlayingScene()
{
    if (mTutorialController) {
        mTutorialController->Stop(false);
    }
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Playing);

    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;

    for (Player* player : mGame->GetPlayers()) {
        player->SetInputAvailableTimer(0.15f);
    }
}

void SceneSystem::StartFocusingScene()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Focusing);
}

void SceneSystem::StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer)
{
    mTalkController->StartTalkWithNPC(talkingNPC, talkingPlayer);
}

bool SceneSystem::TryStartTutorial(
    const std::string& tutorialId,
    Player* tutorialPlayer)
{
    return mTutorialController &&
           mTutorialController->TryStart(
               tutorialId,
               tutorialPlayer);
}

bool SceneSystem::PreviewTutorial(
    const std::string& tutorialId)
{
    return mTutorialController &&
           mTutorialController->Preview(tutorialId);
}

Player* SceneSystem::GetTalkingPlayer() const
{
    if (mTutorialController &&
        mTutorialController->HasActiveTutorial()) {
        return mTutorialController->GetTutorialPlayer();
    }
    return mTalkingPlayer;
}

void SceneSystem::StartFadeIn()
{
    mTransitionController->StartFadeIn();
}

bool SceneSystem::RequestPlayerRespawn(Player* player)
{
    if (!player) {
        return false;
    }

    const bool requested = mTransitionController->RequestFadeAction([this, player]() {
        player->RespawnAtRestartPoint();
        StartPlayingScene();
    });

    if (!requested) {
        return false;
    }

    player->SetVelocity(glm::vec3(0.0f));
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Focusing);
    return true;
}

void SceneSystem::RequestStageChange(int stageNum)
{
    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mTransitionController->RequestStageChange(stageNum);
}

void SceneSystem::OnBoatArrived(Boat* boat)
{
    Stage* currentStage = mGame->GetCurrentStage();
    if (!currentStage) {
        return;
    }

    Player* mainPlayer = mGame->GetMainPlayer();
    for (Player* player : mGame->GetPlayers()) {
        if (!player) {
            continue;
        }

        const bool isInactiveSoloClone =
            !mGame->GetIsPlayer2Joined() &&
            !mGame->GetIsPlayerSplit() &&
            player != mainPlayer;
        if (isInactiveSoloClone) {
            continue;
        }

        player->OnBoatArrived(boat);
    }

    mTutorialController->TryStartBattleTutorial();
    mTutorialController->TryStartJustDodgeTutorial();
}

void SceneSystem::OnStageClear()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::StageClear);

    Mix_HaltMusic();

    if (mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("clear_se");
    }

    mClearTimer = 12.0f;
}

void SceneSystem::OnEnemyLaunched()
{
    mTutorialController->OnEnemyLaunched();
}

void SceneSystem::OnStrongAttacked()
{
    mTutorialController->OnStrongAttacked();
}

void SceneSystem::OnLanded()
{
    mTutorialController->OnLanded();
}

void SceneSystem::OnPlayerDied()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::GameOver);
}

void SceneSystem::UpdateClearTimer(float deltaTime)
{
    if (mClearTimer < 0.0f) {
        return;
    }

    mClearTimer -= deltaTime;

    if (mClearTimer < 0.0f) {
        // Stop this timer before requesting the transition so that the fade
        // timer is not restarted on every frame.
        mClearTimer = -1.0f;
        RequestStageChange(0);
    }
}
