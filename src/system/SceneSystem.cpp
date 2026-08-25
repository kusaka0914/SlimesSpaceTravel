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
#include "system/sequence/SequenceSystem.h"
#include "system/ending/EndingRollConfig.h"

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
    if (IsGameOver() && mFadeTimer < 0.0f && mGameOverTimer >= 0.0f) {
        mGameOverTimer -= std::max(0.0f, deltaTime);
        if (mGameOverTimer < 0.0f) {


            mGameOverTimer = -1.0f;
            RestartGame();
        }
    }
    if (IsCredits() && mFadeTimer < 0.0f) {
        mCreditsElapsed += deltaTime;
        EndingRollConfig endingRoll;
        EndingRollConfigIO::Load(endingRoll);
        if (mCreditsElapsed >= endingRoll.totalDuration) {
            FinishCredits();
        }
    }
    mTutorialController->Update(deltaTime);
    mTalkController->Update(deltaTime);
    UpdateClearTimer(deltaTime);
    UpdateArrivalTutorials();
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

    case GameProgressState::SceneState::Ending:
        mUIState->IncTalkUIIndex();
        return true;

    case GameProgressState::SceneState::Credits:



        return true;

    case GameProgressState::SceneState::Talking:
    {


        const Player* talkingPlayer = GetTalkingPlayer();
        if (talkingPlayer && talkingPlayer->GetPlayerNum() != playerNum) {
            return false;
        }
        if (mTutorialController->HasActiveTutorial()) {
            mTutorialController->TryAdvanceFromConfirm();
        } else {
            mTalkController->TryAdvanceTalkFromConfirm();
        }
        return true;
    }

    case GameProgressState::SceneState::Playing:
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



        if (mHasOpeningResume) {
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
    mTransitionController->StartOpening();
}

void SceneSystem::StartEnding()
{
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
    mCreditsElapsed = 0.0f;
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
    mGameOverTimer = -1.0f;
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
    mGameOverTimer = -1.0f;

    mTalkingNPC = nullptr;
    mTalkingPlayer = nullptr;
    mIsFinishingOpeningStory = false;





    mHasPendingForcedArrivalTalk = !mSuppressForcedArrivalTalkOnce;
    mSuppressForcedArrivalTalkOnce = false;
    mHasReachedArrivalDestination = false;

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

bool SceneSystem::StartOpeningAfterTalkPage(
    NPC* talkingNPC, Player* talkingPlayer, int resumeTalkPageIndex,
    std::size_t sourceTalkPageIndex)
{
    if (!talkingNPC || !talkingPlayer || resumeTalkPageIndex < 0 ||
        !mGame ||
        mGame->HasCompletedNPCOpeningTrigger(
            talkingNPC, sourceTalkPageIndex)) {
        return false;
    }

    mOpeningReturnStageNum = mGame->GetCurrentStageNum();
    mOpeningReturnStageYamlPath = mGame->GetCurrentStageYamlPath();
    mOpeningResumeNPCConversationId =
        mGame->GetNPCConversationId(talkingNPC);
    mOpeningResumePlayerIndex = 0;
    const std::vector<Player*>& players = mGame->GetPlayers();
    for (std::size_t index = 0; index < players.size(); ++index) {
        if (players[index] == talkingPlayer) {
            mOpeningResumePlayerIndex = static_cast<int>(index);
            break;
        }
    }



    mGame->MarkNPCOpeningTriggerCompleted(
        talkingNPC, sourceTalkPageIndex);

    // オープニングは全画面UIなので別ステージを読込む必要がない。同期読込によるフェード開始前の停止を避ける。






    mOpeningResumeNPC = nullptr;
    mOpeningResumePlayer = nullptr;
    mOpeningResumeTalkPageIndex = resumeTalkPageIndex;
    mHasOpeningResume = true;
    mIsFinishingOpeningStory = false;
    mUIState->StartTalkWith(UIState::TalkWith::Opening);
    StartOpening();
    return true;
}

void SceneSystem::FinishOpeningStory()
{

    // 最終ページ後は毎フレーム呼ばれるため、最初の遷移要求を後続フレームで上書きしない。



    if (mIsFinishingOpeningStory) {
        return;
    }
    mIsFinishingOpeningStory = true;

    if (!mHasOpeningResume || mOpeningReturnStageNum < 0 ||
        mOpeningReturnStageYamlPath.empty() ||
        mOpeningResumeNPCConversationId.empty() ||
        mOpeningResumeTalkPageIndex < 0) {
        mIsFinishingOpeningStory = false;
        StartFadeIn();
        return;
    }

    const int resumeTalkPageIndex = mOpeningResumeTalkPageIndex;
    const int returnStageNum = mOpeningReturnStageNum;
    const int resumePlayerIndex = mOpeningResumePlayerIndex;
    const std::string returnStageYamlPath = mOpeningReturnStageYamlPath;
    const std::string resumeNPCConversationId =
        mOpeningResumeNPCConversationId;
    mOpeningResumeNPC = nullptr;
    mOpeningResumePlayer = nullptr;
    mOpeningResumeTalkPageIndex = -1;
    mOpeningReturnStageNum = -1;
    mOpeningReturnStageYamlPath.clear();
    mOpeningResumeNPCConversationId.clear();
    mHasOpeningResume = false;
    mSuppressForcedArrivalTalkOnce = true;

    if (!RequestFadeAction([this, returnStageNum, returnStageYamlPath,
                            resumeNPCConversationId, resumePlayerIndex,
                            resumeTalkPageIndex]() {
        const bool isAlreadyOnReturnStage =
            mGame->GetCurrentStageNum() == returnStageNum &&
            mGame->GetCurrentStageYamlPath() == returnStageYamlPath;
        if (!isAlreadyOnReturnStage &&
            !mGame->LoadStageForScene(returnStageNum, returnStageYamlPath)) {
            mIsFinishingOpeningStory = false;
            StartPlayingScene();
            return;
        }

        NPC* resumeNPC =
            mGame->FindNPCByConversationId(resumeNPCConversationId);
        const std::vector<Player*>& players = mGame->GetPlayers();
        Player* resumePlayer =
            resumePlayerIndex >= 0 &&
                    resumePlayerIndex < static_cast<int>(players.size())
                ? players[resumePlayerIndex]
                : mGame->GetMainPlayer();
        if (!resumeNPC || !resumePlayer || !resumeNPC->GetIsActive() ||
            !resumePlayer->GetIsActive()) {
            mIsFinishingOpeningStory = false;
            StartPlayingScene();
            return;
        }
        mIsFinishingOpeningStory = false;
        mTalkController->ResumeTalkWithNPC(
            resumeNPC, resumePlayer, resumeTalkPageIndex);
    })) {
        mIsFinishingOpeningStory = false;
    }
}

bool SceneSystem::StartEndingAfterTalkPage(
    NPC* talkingNPC, std::size_t sourceTalkPageIndex)
{
    if (!talkingNPC || !mGame ||
        !mGame->AreAllMainStagesCleared() ||
        mGame->HasCompletedNPCEndingTrigger(
            talkingNPC, sourceTalkPageIndex)) {
        return false;
    }

    mGame->MarkNPCEndingTriggerCompleted(
        talkingNPC, sourceTalkPageIndex);
    mUIState->StartTalkWith(UIState::TalkWith::Ending);
    StartEnding();
    return true;
}

void SceneSystem::FinishEndingStory()
{
    RequestFadeAction([this]() { StartCredits(); });
}

void SceneSystem::StartCredits()
{
    mUIState->FinishTalkWith();
    mCreditsElapsed = 0.0f;
    mGameProgressState->SetCurrentSceneState(
        GameProgressState::SceneState::Credits);
}

void SceneSystem::FinishCredits()
{
    RequestFadeAction([this]() {
        mCreditsElapsed = 0.0f;



        mGame->MarkEndingRollCompleted();
        mUIState->FinishTalkWith();
        mGameProgressState->SetCurrentSceneState(
            GameProgressState::SceneState::Title);
    });
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
    mOpeningResumeNPC = nullptr;
    mOpeningResumePlayer = nullptr;
    mOpeningResumeTalkPageIndex = -1;
    mHasOpeningResume = false;
    mOpeningReturnStageNum = -1;
    mOpeningReturnStageYamlPath.clear();
    mOpeningResumeNPCConversationId.clear();
    mHasPendingForcedArrivalTalk = true;
    mHasReachedArrivalDestination = false;
    mSuppressForcedArrivalTalkOnce = false;
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

    // 到着直後には1人分裂の合体が続くため、入力の所有者が確定した次フレームでチュートリアルを始める。




    mHasPendingArrivalTutorials = true;
}

void SceneSystem::UpdateArrivalTutorials()
{
    if (!mHasPendingArrivalTutorials || !IsPlaying()) {
        return;
    }

    Player* controlledPlayer = mGame->GetControlledPlayer();
    if (!controlledPlayer || !controlledPlayer->GetIsActive() ||
        !controlledPlayer->GetOnGround()) {
        return;
    }

    mHasPendingArrivalTutorials = false;
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
    const auto findInPlanet = [this](Planet* planet) -> NPC* {
        if (!planet) {
            return nullptr;
        }
        for (NPC* npc : planet->GetNPCs()) {
            if (npc && npc->GetIsActive() &&
                npc->GetForcesTalkOnArrival() &&
                !npc->GetResolvedTalkTexts().empty() &&
                !mGame->HasShownNPCConversation(npc)) {
                return npc;
            }
        }
        return nullptr;
    };

    if (NPC* arrivalNPC = findInPlanet(arrivalPlanet)) {
        return arrivalNPC;
    }

    // ステージ開始演出中は現在惑星が未解決になることがある。1回限りの到着会話を失わないよう、ステージ全体から設定済みNPCを探す。




    Stage* stage = mGame->GetCurrentStage();
    if (stage) {
        for (Planet* planet : stage->GetPlanets()) {
            if (planet != arrivalPlanet) {
                if (NPC* arrivalNPC = findInPlanet(planet)) {
                    return arrivalNPC;
                }
            }
        }
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
