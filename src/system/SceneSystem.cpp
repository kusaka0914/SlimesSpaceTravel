#include "system/SceneSystem.h"

#include "Game.h"
#include "Stage.h"

#include "actor/Boat.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"

#include "system/AudioSystem.h"
#include "system/scene/ArrivalSceneFlow.h"
#include "system/scene/EndingCreditsFlow.h"
#include "system/scene/OpeningStoryFlow.h"
#include "system/scene/SceneTransitionController.h"
#include "system/scene/TalkController.h"
#include "system/scene/TutorialController.h"
#include "system/sequence/SequenceSystem.h"
#include "system/ending/EndingRollConfig.h"
#include "system/story/StorybookConfig.h"
#include "system/UILoadSystem.h"

#include <SDL2/SDL_mixer.h>
#include <glm/glm.hpp>

#include <utility>

SceneSystem::SceneSystem(Game* game, const UILoadSystem& uiLoadSystem)
    : mGame(game),
      mUILoadSystem(uiLoadSystem),
      mFadeTimer(-1.0f),
      mClearTimer(-1.0f),
      mIsFadeOut(false),
      mHasPendingStageChange(false),
      mNextStageNum(-1)
{
    mGameProgressState = std::make_unique<GameProgressState>();
    mUIState = std::make_unique<UIState>();
    mStorybookConfig = std::make_unique<StorybookConfig>();
    ReloadStorybookConfig();

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
    mArrivalSceneFlow = std::make_unique<ArrivalSceneFlow>(
        *mGame,
        *this,
        *mTalkController,
        *mTutorialController);
    mOpeningStoryFlow = std::make_unique<OpeningStoryFlow>(
        *mGame,
        *this,
        *mUIState,
        *mTalkController,
        [this]() { mArrivalSceneFlow->SuppressForcedTalkOnce(); });
    mEndingCreditsFlow = std::make_unique<EndingCreditsFlow>(
        *mGame,
        *this,
        *mGameProgressState,
        *mUIState);
}

void SceneSystem::Update(float deltaTime)
{
    mTransitionController->UpdateFade(deltaTime);
    if (IsGameOver() && mFadeTimer < 0.0f && mGameOverTimer >= 0.0f) {
        mGameOverTimer -= std::max(0.0f, deltaTime);
        if (mGameOverTimer < 0.0f) {


            mGameOverTimer = -1.0f;
            RestartGame();
        }
    }
    mEndingCreditsFlow->UpdateCredits(deltaTime);
    mTutorialController->Update(deltaTime);
    mTalkController->Update(deltaTime);
    AdvanceOpeningStoryIfComplete();
    FinishEndingStoryIfComplete();
    UpdateClearTimer(deltaTime);
    mArrivalSceneFlow->Update();
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

    case GameProgressState::SceneState::Ending:
        mUIState->IncTalkUIIndex();
        return true;

    case GameProgressState::SceneState::Credits:



        return true;

    case GameProgressState::SceneState::Talking:
    {
        if (mTutorialController->HasActiveTutorial()) {
            mTutorialController->TryAdvanceFromConfirm();
            return true;
        }

        const Player* talkingPlayer = GetTalkingPlayer();
        if (talkingPlayer && talkingPlayer->GetPlayerNum() != playerNum) {
            return false;
        }
        mTalkController->TryAdvanceTalkFromConfirm();
        return true;
    }

    case GameProgressState::SceneState::Playing:
        if (mTutorialController->HasActiveTutorial()) {
            // 操作目標の実行中はA/Spaceも通常操作に使うため、
            // 同じ入力で別の会話を開始しない。
            return false;
        }
        return mTalkController->TryStartTalkWithNPC(playerNum);

    case GameProgressState::SceneState::GameOver:

        return false;

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



        if (mOpeningStoryFlow->HasResume()) {
            FinishOpeningStory();
        } else {
            StartFadeIn();
        }
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
    ReloadStorybookConfig();
    mTransitionController->StartOpening();
}

bool SceneSystem::IsShowingTutorialConversation() const
{
    return mTutorialController &&
           mTutorialController->IsShowingConversation();
}

bool SceneSystem::IsShowingTutorialObjective() const
{
    return mTutorialController &&
           mTutorialController->IsShowingActionObjective();
}

void SceneSystem::StartEnding()
{
    ReloadStorybookConfig();
    mUIState->StartTalkWith(UIState::TalkWith::Ending);
    mTransitionController->StartEnding();
}

void SceneSystem::StartBattleStyleSelection()
{
    if (mGame->HasSelectedPlayerControlStyle()) {

        RequestStageChange(mGame->IsStageCleared(1) ? 0 : 1);
        return;
    }

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


    RequestStageChange(mGame->IsStageCleared(1) ? 0 : 1);
}

void SceneSystem::DebugEnterTitle()
{
    ResetForDebugScene(GameProgressState::SceneState::Title);
}

void SceneSystem::EnterTitleAtFadeMidpoint()
{
    ApplyDebugSceneState(GameProgressState::SceneState::Title);
}

void SceneSystem::DebugEnterOpening()
{
    ResetForDebugScene(GameProgressState::SceneState::Opening);
}

void SceneSystem::DebugEnterEnding()
{
    ResetForDebugScene(GameProgressState::SceneState::Ending);
    mUIState->StartTalkWith(UIState::TalkWith::Ending);
}

void SceneSystem::DebugStartCredits()
{
    ResetForDebugScene(GameProgressState::SceneState::Credits);
    mEndingCreditsFlow->Reset();
}

void SceneSystem::ClearActorReferencesForStageReload()
{
    if (mTutorialController) {
        mTutorialController->Stop(false);
    }
    if (mTalkController) {
        mTalkController->ClearActorReferencesForStageReload();
    } else {
        mTalkingNPC = nullptr;
        mTalkingPlayer = nullptr;
    }
}

void SceneSystem::ResetForDebugScene(
    GameProgressState::SceneState destinationScene)
{
    mTransitionController->CancelPendingTransition();
    ApplyDebugSceneState(destinationScene);
}

void SceneSystem::ApplyDebugSceneState(
    GameProgressState::SceneState destinationScene)
{
    if (mTutorialController) {
        mTutorialController->Stop(false);
    }

    if (mClearAudioChannel >= 0) {
        Mix_HaltChannel(mClearAudioChannel);
    }
    mClearAudioChannel = -1;
    mClearTimer = -1.0f;
    mGameOverTimer = -1.0f;
    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mOpeningStoryFlow->Reset();
    mEndingCreditsFlow->Reset();
    mArrivalSceneFlow->Reset();

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
    mGameOverTimer = -1.0f;

    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mOpeningStoryFlow->PreparePlayingScene();
    mArrivalSceneFlow->PreparePlayingScene();

    for (Player* player : mGame->GetPlayers()) {
        player->SetInputAvailableTimer(0.15f);
    }
}

void SceneSystem::FinishFocusingScene()
{
    if (!mTutorialController ||
        !mTutorialController->ResumeAfterFocus()) {
        StartPlayingScene();
        return;
    }

    if (!IsPlaying()) {
        return;
    }

    for (Player* player : mGame->GetPlayers()) {
        if (player) {
            player->SetInputAvailableTimer(0.15f);
        }
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

bool SceneSystem::StartOpeningAfterTalkPage(
    NPC* talkingNPC, Player* talkingPlayer, int resumeTalkPageIndex,
    std::size_t sourceTalkPageIndex)
{
    return mOpeningStoryFlow->StartAfterTalkPage(
        talkingNPC,
        talkingPlayer,
        resumeTalkPageIndex,
        sourceTalkPageIndex);
}

void SceneSystem::FinishOpeningStory()
{
    mOpeningStoryFlow->Finish();
}

bool SceneSystem::StartEndingAfterTalkPage(
    NPC* talkingNPC, std::size_t sourceTalkPageIndex)
{
    return mEndingCreditsFlow->StartEndingAfterTalkPage(
        talkingNPC,
        sourceTalkPageIndex);
}

void SceneSystem::FinishEndingStory()
{
    mEndingCreditsFlow->FinishEnding();
}

void SceneSystem::StartCredits()
{
    mEndingCreditsFlow->StartCredits();
}

void SceneSystem::FinishCredits()
{
    mEndingCreditsFlow->FinishCredits();
}

void SceneSystem::ReloadStorybookConfig()
{
    StorybookConfig loadedConfig;
    if (loadedConfig.Load()) {
        *mStorybookConfig = std::move(loadedConfig);
    }
}

void SceneSystem::ReloadEndingRollConfig()
{
    mEndingCreditsFlow->ReloadConfig();
}

std::string SceneSystem::FindStorybookPageImage(
    const std::string& trackId,
    int pageIndex) const
{
    return mStorybookConfig
        ? mStorybookConfig->GetPageImage(trackId, pageIndex)
        : std::string{};
}

const EndingRollConfig& SceneSystem::GetEndingRollConfig() const
{
    return mEndingCreditsFlow->GetConfig();
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

    const int playerNum = player->GetPlayerNum();
    const bool requested = mTransitionController->RequestFadeAction([this, playerNum]() {
        Player* respawningPlayer = nullptr;
        for (Player* currentPlayer : mGame->GetPlayers()) {
            if (currentPlayer && currentPlayer->GetPlayerNum() == playerNum) {
                respawningPlayer = currentPlayer;
                break;
            }
        }

        if (respawningPlayer) {
            respawningPlayer->RespawnAtRestartPoint();
        }
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
    mOpeningStoryFlow->PrepareStageChange();
    mArrivalSceneFlow->PrepareStageChange();
    mTransitionController->RequestStageChange(stageNum);
}

void SceneSystem::AdvanceOpeningStoryIfComplete()
{
    if (!IsOpening() || mFadeTimer >= 0.0f) {
        return;
    }

    const UIState::TalkWith talkWith = mUIState->GetCurrentTalkWith();
    const char* textId = nullptr;
    switch (talkWith) {
    case UIState::TalkWith::Opening:
        textId = "openingText";
        break;
    case UIState::TalkWith::Mother:
        textId = "talkWithMotherText";
        break;
    case UIState::TalkWith::Doctor:
        textId = "talkWithDoctorText";
        break;
    default:
        return;
    }

    if (mUIState->GetTalkUIIndex() <
        GetSceneTalkPageCount("opening", textId)) {
        return;
    }

    switch (talkWith) {
    case UIState::TalkWith::Opening:
        mUIState->StartTalkWith(UIState::TalkWith::Mother);
        break;
    case UIState::TalkWith::Mother:
        mUIState->StartTalkWith(UIState::TalkWith::Doctor);
        break;
    case UIState::TalkWith::Doctor:
        FinishOpeningStory();
        break;
    default:
        break;
    }
}

void SceneSystem::FinishEndingStoryIfComplete()
{
    if (!IsEnding() || mFadeTimer >= 0.0f ||
        mUIState->GetCurrentTalkWith() != UIState::TalkWith::Ending) {
        return;
    }

    if (mUIState->GetTalkUIIndex() <
        GetSceneTalkPageCount("ending", "endingText")) {
        return;
    }

    FinishEndingStory();
}

int SceneSystem::GetSceneTalkPageCount(
    const char* sceneName,
    const char* textId) const
{
    const UILoadSystem::TextInfo* textInfo =
        mUILoadSystem.GetTextInfo(sceneName, textId);
    return textInfo
        ? static_cast<int>(textInfo->texts.size())
        : 0;
}

float SceneSystem::GetCreditsElapsed() const
{
    return mEndingCreditsFlow
        ? mEndingCreditsFlow->GetCreditsElapsed()
        : 0.0f;
}

void SceneSystem::OnBoatArrived(Boat* boat)
{
    mArrivalSceneFlow->OnBoatArrived(boat);
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

void SceneSystem::OnUGCStageClear()
{
    mGameProgressState->SetCurrentSceneState(
        GameProgressState::SceneState::StageClear);
    Mix_HaltMusic();
    if (mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("clear_se");
    }

    // UGCは星の獲得演出完了を起点に専用結果画面へ遷移するため、
    // SceneSystemの通常ステージ用タイマーでは拠点へ戻さない。
    mClearTimer = -1.0f;
    mClearAudioChannel = -1;
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
    if (IsGameOver()) {
        return;
    }

    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::GameOver);
    mGameOverTimer = 3.0f;
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
    // 初回はエンディングからスタッフロールを再生する。完走した事実は
    // ステージ進行とは別の専用保存値に記録され、以後の再クリアでは
    // 拠点へ戻る。
    if (mGame->GetCurrentStageNum() == 5 &&
        !mGame->HasCompletedEndingRoll()) {
        StartEnding();
        return;
    }
    RequestStageChange(0);
}
