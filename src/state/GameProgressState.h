#pragma once

#include <string>

class GameProgressState {
public:
    enum class SceneState {
        None,
        Title,
        BattleStyleSelection,
        Opening,
        Ending,
        Credits,
        Talking,
        Playing,
        Focusing,
        ShowUI,
        StageClear,
        GameOver,
        GameClear
    };

    GameProgressState();

    void SetIsFirstBreak(bool isFirstBreak) { mIsFirstBreak = isFirstBreak; }
    void SetIsFirstStrongAttack(bool isFirstStrongAttack) { mIsFirstStrongAttack = isFirstStrongAttack; }

    void SetCurrentSceneState(SceneState currentSceneState) { mCurrentSceneState = currentSceneState; }
    void SetNextSceneState(SceneState nextSceneState) { mNextSceneState = nextSceneState; }

    bool GetIsFirstBreak() const { return mIsFirstBreak; }
    bool GetIsFirstStrongAttack() const { return mIsFirstStrongAttack; }

    SceneState GetSceneState() const { return mCurrentSceneState; }
    SceneState GetNextSceneState() const { return mNextSceneState; }
private:
    bool mIsFirstBreak;
    bool mIsFirstStrongAttack;

    SceneState mCurrentSceneState;
    SceneState mNextSceneState;
};
