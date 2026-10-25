#pragma once

#include <functional>

class Game;
class GameProgressState;
class UIState;

// SceneSystemの遷移状態を借用し、フェード中点・完了時の処理とStage切替を実行する。
class SceneTransitionController {
public:
    SceneTransitionController(Game* game, GameProgressState* gameProgressState, UIState* uiState, float& fadeTimer,
                              bool& isFadeOut, bool& hasPendingStageChange, int& nextStageNum);

    void UpdateFade(float deltaTime);
    void StartOpening();
    void StartEnding();
    void StartBattleStyleSelection();
    void StartFadeIn();

    // 中点・完了コールバックとStage変更を含む遷移予約を取り消す。
    void CancelPendingTransition();
    void RequestStageChange(int stageNum);

    // 空の中点処理・遷移中はfalse。受理した処理はフェード中点と完了時に遅延実行する。
    // コールバックの参照先は実行完了まで有効であること。
    bool RequestFadeAction(std::function<void()> midpointAction, std::function<void()> completionAction = {});

private:
    void ApplySceneChange();

    Game* mGame;
    GameProgressState* mGameProgressState;
    UIState* mUIState;

    float& mFadeTimer;
    bool& mIsFadeOut;
    bool& mHasPendingStageChange;
    int& mNextStageNum;

    std::function<void()> mMidpointAction;
    std::function<void()> mFadeCompletionAction;
};
