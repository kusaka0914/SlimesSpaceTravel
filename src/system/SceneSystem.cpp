#include "system/SceneSystem.h"

#include "Game.h"

#include "actor/Boat.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "system/AudioSystem.h"
#include "system/scene/SceneTransitionController.h"
#include "system/scene/TalkController.h"
#include "system/scene/TutorialController.h"
#include "system/sequence/SequenceSystem.h"

#include <SDL2/SDL_mixer.h>
#include <glm/glm.hpp>

#include <utility>

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
    UpdateForcedArrivalTalk();
}

bool SceneSystem::OnConfirmPressed(int playerNum)
{
    if (mFadeTimer >= 0.0f) {
        return false;
    }

    const auto sceneState = mGameProgressState->GetSceneState();

    switch (sceneState) {
    case GameProgressState::SceneState::Title:
        StartBattleStyleSelection();
        return true;

    case GameProgressState::SceneState::BattleStyleSelection:
        ConfirmBattleStyleSelection();
        return true;

    case GameProgressState::SceneState::Opening:
        mTalkController->TryAdvanceTalkFromConfirm();
        return true;

    case GameProgressState::SceneState::Talking:
        if (mTutorialController->HasActiveTutorial()) {
            mTutorialController->TryAdvanceFromConfirm();
        } else {
            mTalkController->TryAdvanceTalkFromConfirm();
        }
        return true;

    case GameProgressState::SceneState::Playing:
        return mTalkController->TryStartTalkWithNPC(playerNum);

    case GameProgressState::SceneState::GameOver:
        RestartGame();
        return true;

    default:
        return false;
    }

    return false;
}

bool SceneSystem::IsWaitingForTutorialPlayerAction() const
{
    return (mTutorialController &&
            mTutorialController->IsWaitingForPlayerAction()) ||
           (mTalkController &&
            mTalkController->IsWaitingForPlayerAction());
}

bool SceneSystem::CanStartTalkWithNPC(const Player* player) const
{
    return mTalkController &&
           mTalkController->CanStartTalkWithNPC(player);
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

void SceneSystem::StartBattleStyleSelection()
{
    mSelectedBattleStyle = PlayerControlStyle::Assist;
    mTransitionController->StartBattleStyleSelection();
}

void SceneSystem::MoveBattleStyleSelection(int direction)
{
    if (!IsBattleStyleSelection() || direction == 0) {
        return;
    }

    mSelectedBattleStyle =
        mSelectedBattleStyle == PlayerControlStyle::Assist
            ? PlayerControlStyle::Standard
            : PlayerControlStyle::Assist;
}

void SceneSystem::ConfirmBattleStyleSelection()
{
    if (!IsBattleStyleSelection()) {
        return;
    }

    mGame->SetPlayerControlStyle(mSelectedBattleStyle);
    RequestStageChange(1);
}

void SceneSystem::DebugEnterTitle()
{
    ResetForDebugScene(GameProgressState::SceneState::Title);
}

void SceneSystem::DebugEnterOpening()
{
    ResetForDebugScene(GameProgressState::SceneState::Opening);
}

void SceneSystem::ResetForDebugScene(
    GameProgressState::SceneState destinationScene)
{
    mTransitionController->CancelPendingTransition();
    if (mTutorialController) {
        mTutorialController->Stop(false);
    }

    if (mClearAudioChannel >= 0) {
        Mix_HaltChannel(mClearAudioChannel);
    }
    mClearAudioChannel = -1;
    mClearTimer = -1.0f;
    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mHasPendingForcedArrivalTalk = false;
    mHasReachedArrivalDestination = false;

    mUIState->StartTalkWith(UIState::TalkWith::Opening);
    mGameProgressState->SetCurrentSceneState(destinationScene);
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

bool SceneSystem::RequestFadeAction(
    std::function<void()> midpointAction,
    std::function<void()> completionAction)
{
    return mTransitionController->RequestFadeAction(
        std::move(midpointAction),
        std::move(completionAction));
}

void SceneSystem::RequestStageChange(int stageNum)
{
    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mHasPendingForcedArrivalTalk = true;
    mHasReachedArrivalDestination = false;
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

    mHasPendingForcedArrivalTalk = true;
    mHasReachedArrivalDestination = true;

    mTutorialController->TryStartBattleTutorial();
    mTutorialController->TryStartJustDodgeTutorial();
}

void SceneSystem::UpdateForcedArrivalTalk()
{
    if (!mHasPendingForcedArrivalTalk || !IsPlaying() ||
        mFadeTimer >= 0.0f || mIsFadeOut) {
        return;
    }

    SequenceSystem* sequenceSystem = mGame->GetSequenceSystem();
    if (sequenceSystem && sequenceSystem->IsPlaying()) {
        return;
    }

    Player* talkingPlayer = mGame->GetControlledPlayer();
    if (!talkingPlayer || !talkingPlayer->GetIsActive() ||
        (mHasReachedArrivalDestination &&
         !talkingPlayer->GetOnGround())) {
        return;
    }

    mHasPendingForcedArrivalTalk = false;
    mHasReachedArrivalDestination = false;
    NPC* arrivalNPC = FindForcedArrivalTalkNPC();
    if (arrivalNPC) {
        StartTalkWithNPC(arrivalNPC, talkingPlayer);
    }
}

NPC* SceneSystem::FindForcedArrivalTalkNPC() const
{
    Player* controlledPlayer = mGame->GetControlledPlayer();
    Planet* arrivalPlanet =
        controlledPlayer ? controlledPlayer->GetCurrentPlanet() : nullptr;
    if (!arrivalPlanet) {
        return nullptr;
    }

    for (NPC* npc : arrivalPlanet->GetNPCs()) {
        if (!npc || !npc->GetIsActive() ||
            !npc->GetForcesTalkOnArrival() ||
            npc->GetResolvedTalkTexts().empty() ||
            mGame->HasShownNPCConversation(npc)) {
            continue;
        }
        return npc;
    }
    return nullptr;
}

void SceneSystem::OnStageClear()
{
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::StageClear);

    Mix_HaltMusic();

    if (mGame->GetAudioSystem()) {
        mClearAudioChannel =
            mGame->GetAudioSystem()->PlaySE("clear_se");
    } else {
        mClearAudioChannel = -1;
    }

    // 音声を読み込めなかった場合だけ、従来の待ち時間を安全策として使う。
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

    AudioSystem* audioSystem = mGame->GetAudioSystem();
    if (mClearAudioChannel >= 0 && audioSystem &&
        audioSystem->IsSEPlaying(mClearAudioChannel)) {
        return;
    }

    if (mClearAudioChannel < 0) {
        mClearTimer -= deltaTime;
        if (mClearTimer >= 0.0f) {
            return;
        }
    }

    // 遷移要求を毎フレーム繰り返さないよう、先に再生状態をリセットする。
    mClearTimer = -1.0f;
    mClearAudioChannel = -1;
    RequestStageChange(0);
}
