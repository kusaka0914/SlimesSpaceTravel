#include "system/SceneSystem.h"

#include "Game.h"
#include "SceneSystem.h"
#include "Stage.h"

#include "actor/Boat.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "state/GameProgressState.h"
#include "state/UIState.h"

#include "system/AudioSystem.h"

#include <SDL2/SDL_mixer.h>

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
}

SceneSystem::~SceneSystem() = default;

void SceneSystem::Update(float deltaTime)
{
    UpdateFade(deltaTime);
    UpdateClearTimer(deltaTime);
}

void SceneSystem::OnConfirmPressed()
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
        mUIState->IncTalkUIIndex();
        mGame->GetAudioSystem()->PlaySE("message_se");
        break;

    case GameProgressState::SceneState::Talking:
        mUIState->IncTalkUIIndex();
        mGame->GetAudioSystem()->PlaySE("message_se");
        break;

    case GameProgressState::SceneState::Playing:
        TryStartTalkWithNPC();
        break;

    case GameProgressState::SceneState::GameOver:
        RestartGame();

    default:
        break;
    }
}

void SceneSystem::OnStartPressed()
{
    if (mGameProgressState->GetSceneState() == GameProgressState::SceneState::Opening) {
        StartFadeIn();
        return;
    }

    bool operationUIShow = mUIState->GetIsOperationUIShow();
    mUIState->SetIsOperationUIShow(!operationUIShow);
}

void SceneSystem::StartOpening()
{
    mFadeTimer = 1.0f;
    mIsFadeOut = false;
    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Opening);
}

void SceneSystem::RestartGame()
{
    StartPlayingScene();
    mGame->RestartGame();
}

void SceneSystem::StartPlayingScene()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Playing);

    for (Player* player : mGame->GetPlayers()) {
        player->SetInputAvailableTimer(0.15f);
    }
}

void SceneSystem::StartFocusingScene()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Focusing);
}

void SceneSystem::StartTalkWithNPC()
{
    mUIState->SetCurrentTalkWith(UIState::TalkWith::NPC);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void SceneSystem::StartFadeIn()
{
    mFadeTimer = 1.0f;
    mIsFadeOut = false;

    mGameProgressState->SetNextSceneState(GameProgressState::SceneState::Playing);
    mUIState->OnFadeIn();
}

void SceneSystem::RequestStageChange(int stageNum)
{
    mNextStageNum = stageNum;
    mHasPendingStageChange = true;

    mFadeTimer = 1.0f;
    mIsFadeOut = false;
}

void SceneSystem::OnBoatArrived(Boat* boat)
{
    Stage* currentStage = mGame->GetCurrentStage();
    if (!currentStage) {
        return;
    }

    std::vector<Planet*> planets = currentStage->GetPlanets();

    for (Player* player : mGame->GetPlayers()) {
        const int nextPlanetIndex = player->GetCurrentPlanetNum() + 1;

        if (nextPlanetIndex >= 0 && nextPlanetIndex < static_cast<int>(planets.size())) {
            player->SetCurrentPlanet(planets[nextPlanetIndex]);
        }

        player->OnBoatArrived(boat);
    }

    TryStartBattleTutorial();
    TryStartJustDodgeTutorial();
}

void SceneSystem::TryStartBattleTutorial()
{
    if (mGame->GetPlayers()[0]->GetCurrentPlanetNum() != 1) {
        return;
    }

    if (mUIState->GetIsBattleTutorialShown()) {
        return;
    }

    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Battle);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mUIState->SetIsBattleTutorialShown(true);
}

void SceneSystem::TryStartJustDodgeTutorial()
{
    if (mGame->GetPlayers()[0]->GetCurrentPlanetNum() != 2) {
        return;
    }

    if (mUIState->GetIsJustDodgeTutorialShown()) {
        return;
    }

    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::JustDodge);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mUIState->SetIsJustDodgeTutorialShown(true);
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
    if (mGameProgressState->GetIsFirstBreak()) {
        return;
    }

    mGameProgressState->SetIsFirstBreak(true);
    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Break);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
}

void SceneSystem::OnStrongAttacked()
{
    if (!mGameProgressState->GetIsFirstBreak() || mGameProgressState->GetIsFirstStrongAttack()) {
        return;
    }

    mGameProgressState->SetIsFirstStrongAttack(true);
}

void SceneSystem::OnLanded()
{
    if (!mGameProgressState->GetIsFirstStrongAttack() || mUIState->GetIsSpecialAttackTutorialShown()) {
        return;
    }

    mUIState->SetIsSpecialAttackTutorialShown(true);
    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Jewel);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
}

void SceneSystem::OnPlayerDied()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::GameOver);
}

void SceneSystem::UpdateFade(float deltaTime)
{
    if (mFadeTimer > -1.0f) {
        mFadeTimer -= deltaTime;

        if (mFadeTimer >= 0.0f || mIsFadeOut) {
            return;
        }

        ApplySceneChange();
    } else if (mIsFadeOut) {
        mIsFadeOut = false;
    }
}

void SceneSystem::UpdateClearTimer(float deltaTime)
{
    if (mClearTimer < 0.0f) {
        return;
    }

    mClearTimer -= deltaTime;

    if (mClearTimer < 0.0f) {
        // StartFadeIn();
        mGame->FinishGame();
    }
}

void SceneSystem::ApplySceneChange()
{
    mIsFadeOut = true;

    const auto nextSceneState = mGameProgressState->GetNextSceneState();

    switch (nextSceneState) {
    case GameProgressState::SceneState::Opening:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Opening);
        mGameProgressState->SetNextSceneState(GameProgressState::SceneState::None);
        break;

    case GameProgressState::SceneState::Playing:
        mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Playing);
        mGameProgressState->SetNextSceneState(GameProgressState::SceneState::None);
        mUIState->SetCurrentTalkWith(UIState::TalkWith::None);
        mGame->ChangeStage(0);
        mUIState->SetTalkUIIndex(0);
        mHasPendingStageChange = true;
        break;

    default:
        break;
    }

    mGame->GetAudioSystem()->TryChangeBGM();

    if (mHasPendingStageChange) {
        mHasPendingStageChange = false;

        mGame->ChangeStage(mNextStageNum);
        mNextStageNum = -1;

        Mix_HaltMusic();
        mGame->ReloadCurrentStage();
    }
}

void SceneSystem::TryStartTalkWithNPC()
{
    Player* mainPlayer = mGame->GetMainPlayer();
    if (!mainPlayer) {
        return;
    }

    NPC* talkableNPC = mainPlayer->GetTalkableNPC();
    if (!talkableNPC) {
        return;
    }

    if (!talkableNPC->GetIsTalkable()) {
        return;
    }

    StartTalkWithNPC();
}