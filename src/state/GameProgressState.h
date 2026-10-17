#pragma once

// 現在のシーン・遷移先と、初回操作チュートリアルの開始済み状態を保持する。シーンの遷移処理は行わない。
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
    // 対応するチュートリアルの開始時に立て、同じ案内の重複開始を防ぐ。
    bool mIsFirstBreak;
    bool mIsFirstStrongAttack;

    SceneState mCurrentSceneState;

    // None は、遷移の予約がない状態を表す。
    SceneState mNextSceneState;
};
