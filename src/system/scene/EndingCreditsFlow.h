#pragma once

#include "system/ending/EndingRollConfig.h"

#include <cstddef>

class Game;
class GameProgressState;
class NPC;
class SceneSystem;
class UIState;

// NPC会話からのエンディング開始とスタッフロール、完走記録・タイトル復帰を調整する。
class EndingCreditsFlow {
public:
    EndingCreditsFlow(Game& game, SceneSystem& sceneSystem, GameProgressState& gameProgressState, UIState& uiState);

    // 全メインStageクリア済みで未実行の会話ページからエンディングを開始する。条件を満たさなければfalse。
    bool StartEndingAfterTalkPage(NPC* talkingNPC, std::size_t sourceTalkPageIndex);
    void FinishEnding();
    void StartCredits();

    // フェード中点で完走を保存し、タイトルへ戻る処理を予約する。
    void FinishCredits();
    void UpdateCredits(float deltaTime);
    void ReloadConfig();
    void Reset() { mCreditsElapsed = 0.0f; }
    float GetCreditsElapsed() const { return mCreditsElapsed; }
    const EndingRollConfig& GetConfig() const { return mConfig; }

private:
    Game& mGame;
    SceneSystem& mSceneSystem;
    GameProgressState& mGameProgressState;
    UIState& mUIState;
    EndingRollConfig mConfig;
    float mCreditsElapsed = 0.0f;
};
