#pragma once

#include "state/GameProgressState.h"
#include "state/UIState.h"

#include <memory>
#include <string>

class Game;
class NPC;
class Player;
class Boat;
class SceneTransitionController;
class TalkController;
class TutorialController;

class SceneSystem {
public:
    explicit SceneSystem(Game* game);
    ~SceneSystem();

    void Update(float deltaTime);

    void OnConfirmPressed(int playerNum = 1);
    void OnStartPressed();

    void RestartGame();
    void StartOpening();
    void StartPlayingScene();
    void StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer);
    bool TryStartTutorial(
        const std::string& tutorialId,
        Player* tutorialPlayer = nullptr);
    bool PreviewTutorial(const std::string& tutorialId);
    void StartFocusingScene();
    void StartFadeIn();
    void StartTalkWith(UIState::TalkWith talkWith) { mUIState->SetCurrentTalkWith(talkWith); }

    bool RequestPlayerRespawn(Player* player);
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
    bool IsOpening() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Opening; }
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

    bool GetHasPendingStageChange() const { return mHasPendingStageChange; }
    float GetFadeTimer() const { return mFadeTimer; }
    UIState::TalkWith GetCurrentTalkWith() const { return mUIState->GetCurrentTalkWith(); }
    int GetTalkUIIndex() const { return mUIState->GetTalkUIIndex(); }

    UIState* GetUIState() { return mUIState.get(); }
    TutorialController* GetTutorialController() const
    {
        return mTutorialController.get();
    }

    NPC* GetTalkingNPC() const { return mTalkingNPC; }
    Player* GetTalkingPlayer() const;

private:
    void CreateControllers();
    void UpdateClearTimer(float deltaTime);

private:
    Game* mGame;

    std::unique_ptr<GameProgressState> mGameProgressState;
    std::unique_ptr<UIState> mUIState;

    std::unique_ptr<SceneTransitionController> mTransitionController;
    std::unique_ptr<TalkController> mTalkController;
    std::unique_ptr<TutorialController> mTutorialController;

    float mFadeTimer;
    float mClearTimer;

    bool mIsFadeOut;
    bool mHasPendingStageChange;
    int mNextStageNum;

    NPC* mTalkingNPC = nullptr;
    Player* mTalkingPlayer = nullptr;
};
