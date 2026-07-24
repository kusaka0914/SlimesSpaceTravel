#pragma once

#include <functional>

class Game;
class GameProgressState;
class UIState;

class SceneTransitionController {
public:
    SceneTransitionController(Game* game, GameProgressState* gameProgressState, UIState* uiState, float& fadeTimer,
                              bool& isFadeOut, bool& hasPendingStageChange, int& nextStageNum);

    void UpdateFade(float deltaTime);
    void StartOpening();
    void StartFadeIn();
    void RequestStageChange(int stageNum);
    bool RequestFadeAction(std::function<void()> midpointAction);

private:
    void ApplySceneChange();

private:
    Game* mGame;
    GameProgressState* mGameProgressState;
    UIState* mUIState;

    float& mFadeTimer;
    bool& mIsFadeOut;
    bool& mHasPendingStageChange;
    int& mNextStageNum;

    std::function<void()> mMidpointAction;
};
