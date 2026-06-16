#pragma once

#include "state/GameProgressState.h"
#include "state/UIState.h"
#include <memory>

class Game;
class UIState;
class Boat;

class SceneSystem {
public:
    SceneSystem(Game* game);
    ~SceneSystem();

    void Update(float deltaTime);

    void OnConfirmPressed();
    void OnStartPressed();

    void RestartGame();
    void StartOpening();
    void StartPlayingScene();
    void StartFocusingScene();
    void StartTalkWithNPC();
    void StartFadeIn();
    void StartTalkWith(UIState::TalkWith talkWith) { mUIState->SetCurrentTalkWith(talkWith); }

    void RequestStageChange(int stageNum);
    void OnBoatArrived(Boat* boat);
    void OnStageClear();
    void OnEnemyLaunched();
    void OnStrongAttacked();
    void OnLanded();
    void OnPlayerDied();

    bool CanUpdateWorld() const { return IsPlaying() || IsFocusing(); };
    bool IsTitle() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Title; }
    bool IsOpening() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Opening; }
    bool IsPlaying() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Playing; }
    bool IsFocusing() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::Focusing; }
    bool IsStageClear() const
    {
        return mGameProgressState->GetSceneState() == GameProgressState::SceneState::StageClear;
    }
    bool IsGameOver() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::GameOver; }
    bool IsGameClear() const { return mGameProgressState->GetSceneState() == GameProgressState::SceneState::GameClear; }
    bool IsTalkWithOpening() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Opening; }
    bool IsTalkWithMother() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Mother; }
    bool IsTalkWithDoctor() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::Doctor; }
    bool IsTalkWithNPC() const { return mUIState->GetCurrentTalkWith() == UIState::TalkWith::NPC; }
    bool IsBattleTutorialShowing() const { return mUIState->GetCurrentTutorialKind() == UIState::TutorialKind::Battle; }
    bool IsBreakTutorialShowing() const { return mUIState->GetCurrentTutorialKind() == UIState::TutorialKind::Break; }
    bool IsJewelTutorialShowing() const { return mUIState->GetCurrentTutorialKind() == UIState::TutorialKind::Jewel; }
    bool IsJustDodgeTutorialShowing() const
    {
        return mUIState->GetCurrentTutorialKind() == UIState::TutorialKind::JustDodge;
    }

    bool GetHasPendingStageChange() const { return mHasPendingStageChange; }
    float GetFadeTimer() const { return mFadeTimer; }
    UIState::TalkWith GetCurrentTalkWith() const { return mUIState->GetCurrentTalkWith(); }
    UIState::TutorialKind GetCurrentTutorialKind() const { return mUIState->GetCurrentTutorialKind(); }
    int GetTalkUIIndex() const { return mUIState->GetTalkUIIndex(); }

    UIState* GetUIState() { return mUIState.get(); }

private:
    void UpdateFade(float deltaTime);
    void UpdateClearTimer(float deltaTime);
    void ApplySceneChange();

    void TryStartTalkWithNPC();
    void TryStartBattleTutorial();
    void TryStartJustDodgeTutorial();

private:
    Game* mGame;

    std::unique_ptr<GameProgressState> mGameProgressState;
    std::unique_ptr<UIState> mUIState;

    float mFadeTimer;
    float mClearTimer;

    bool mIsFadeOut;
    bool mHasPendingStageChange;
    int mNextStageNum;
};