#pragma once

#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "actor/player/PlayerTypes.h"

#include <functional>
#include <memory>
#include <cstddef>
#include <string>

class Game;
class UILoadSystem;
class NPC;
class Player;
class Boat;
class ArrivalSceneFlow;
class EndingCreditsFlow;
class OpeningStoryFlow;
class SceneTransitionController;
class StorybookConfig;
class TalkController;
class TutorialController;
struct EndingRollConfig;

class SceneSystem {
public:
    SceneSystem(Game* game, const UILoadSystem& uiLoadSystem);
    ~SceneSystem();

    void Update(float deltaTime);

    bool OnConfirmPressed(int playerNum = 1);
    void OnStartPressed();

    void RestartGame();
    void StartOpening();
    void StartEnding();
    void StartBattleStyleSelection();
    void MoveBattleStyleSelection(int direction);
    void ConfirmBattleStyleSelection();
    void DebugEnterTitle();
    void EnterTitleAtFadeMidpoint();
    void DebugEnterOpening();
    void DebugEnterEnding();
    void DebugStartCredits();
    void StartPlayingScene();
    void StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer);
    bool StartOpeningAfterTalkPage(
        NPC* talkingNPC, Player* talkingPlayer, int resumeTalkPageIndex,
        std::size_t sourceTalkPageIndex);
    void FinishOpeningStory();
    bool StartEndingAfterTalkPage(NPC* talkingNPC, std::size_t sourceTalkPageIndex);
    void FinishEndingStory();
    void StartCredits();
    void FinishCredits();
    void ReloadStorybookConfig();
    void ReloadEndingRollConfig();
    bool TryStartTutorial(
        const std::string& tutorialId,
        Player* tutorialPlayer = nullptr);
    bool PreviewTutorial(const std::string& tutorialId);
    void StartFocusingScene();
    void StartFadeIn();
    void StartTalkWith(UIState::TalkWith talkWith) { mUIState->SetCurrentTalkWith(talkWith); }

    bool RequestPlayerRespawn(Player* player);
    bool RequestFadeAction(
        std::function<void()> midpointAction,
        std::function<void()> completionAction = {});
    void RequestStageChange(int stageNum);
    void OnBoatArrived(Boat* boat);
    void OnStageClear();
    void OnEnemyLaunched();
    void OnStrongAttacked();
    void OnLanded();
    void OnPlayerSwitchSucceeded();
    void OnPlayerSplitMergeSucceeded();
    void OnPlayerDied();

    bool CanUpdateWorld() const { return IsPlaying() || IsFocusing(); }
    bool IsTitle() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Title; }
    bool IsBattleStyleSelection() const
    {
        return mGameProgressState->GetSceneState() ==
               GameProgressState::SceneState::BattleStyleSelection;
    }
    bool IsOpening() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Opening; }
    bool IsEnding() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Ending; }
    bool IsCredits() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Credits; }
    bool IsTalking() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Talking; }
    bool IsPlaying() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Playing; }
    bool IsFocusing() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Focusing; }
    bool IsStageClear() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::StageClear; }
    bool IsGameOver() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::GameOver; }
    bool IsGameClear() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::GameClear; }

    bool IsTalkWithOpening() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Opening; }
    bool IsTalkWithMother() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Mother; }
    bool IsTalkWithDoctor() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Doctor; }
    bool IsTalkWithNPC() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::NPC; }
    bool IsWaitingForTutorialPlayerAction() const;
    bool IsWaitingForTutorialPlayerSwitch() const;
    bool IsWaitingForTutorialPlayerJump() const;
    bool IsWaitingForTutorialPlayerSplitMerge() const;

    bool HasActiveTutorial() const;
    bool IsTutorialActive(
        const std::string& tutorialId) const;
    bool CanStartTalkWithNPC(const Player* player) const;

    bool GetHasPendingStageChange() const { return mHasPendingStageChange; }
    float GetFadeTimer() const { return mFadeTimer; }
    bool IsFadingOut() const { return mIsFadeOut; }
    UIState::TalkWith GetCurrentTalkWith() const { return mUIState->GetCurrentTalkWith(); }
    int GetTalkUIIndex() const { return mUIState->GetTalkUIIndex(); }
    float GetCreditsElapsed() const;
    std::string FindStorybookPageImage(
        const std::string& trackId,
        int pageIndex) const;
    const EndingRollConfig& GetEndingRollConfig() const;
    PlayerControlStyle GetSelectedBattleStyle() const
    {
        return mSelectedBattleStyle;
    }

    UIState* GetUIState() { return mUIState.get(); }
    TutorialController* GetTutorialController() const
    {
        return mTutorialController.get();
    }

    NPC* GetTalkingNPC() const { return mTalkingNPC; }
    Player* GetTalkingPlayer() const;

private:
    void CreateControllers();
    void ApplyDebugSceneState(
        GameProgressState::SceneState destinationScene);
    void ResetForDebugScene(
        GameProgressState::SceneState destinationScene);
    void UpdateClearTimer(float deltaTime);
    void AdvanceOpeningStoryIfComplete();
    void FinishEndingStoryIfComplete();
    int GetSceneTalkPageCount(
        const char* sceneName,
        const char* textId) const;

private:
    Game* mGame;
    const UILoadSystem& mUILoadSystem;

    std::unique_ptr<GameProgressState> mGameProgressState;
    std::unique_ptr<UIState> mUIState;

    std::unique_ptr<SceneTransitionController> mTransitionController;
    std::unique_ptr<TalkController> mTalkController;
    std::unique_ptr<TutorialController> mTutorialController;
    std::unique_ptr<OpeningStoryFlow> mOpeningStoryFlow;
    std::unique_ptr<EndingCreditsFlow> mEndingCreditsFlow;
    std::unique_ptr<ArrivalSceneFlow> mArrivalSceneFlow;
    std::unique_ptr<StorybookConfig> mStorybookConfig;

    float mFadeTimer;
    float mClearTimer;
    float mGameOverTimer = -1.0f;
    int mClearAudioChannel = -1;

    bool mIsFadeOut;
    bool mHasPendingStageChange;
    int mNextStageNum;

    NPC* mTalkingNPC = nullptr;
    Player* mTalkingPlayer = nullptr;
    PlayerControlStyle mSelectedBattleStyle = PlayerControlStyle::Assist;
};
