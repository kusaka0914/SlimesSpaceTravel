#pragma once

#include "system/ending/EndingRollConfig.h"

#include <cstddef>

class Game;
class GameProgressState;
class NPC;
class SceneSystem;
class UIState;

class EndingCreditsFlow {
public:
    EndingCreditsFlow(
        Game& game,
        SceneSystem& sceneSystem,
        GameProgressState& gameProgressState,
        UIState& uiState);

    bool StartEndingAfterTalkPage(
        NPC* talkingNPC,
        std::size_t sourceTalkPageIndex);
    void FinishEnding();
    void StartCredits();
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
