#pragma once

#include <cstddef>
#include <functional>
#include <string>

class Game;
class NPC;
class Player;
class SceneSystem;
class TalkController;
class UIState;

class OpeningStoryFlow {
public:
    OpeningStoryFlow(
        Game& game,
        SceneSystem& sceneSystem,
        UIState& uiState,
        TalkController& talkController,
        std::function<void()> suppressForcedArrivalTalk);

    bool StartAfterTalkPage(
        NPC* talkingNPC,
        Player* talkingPlayer,
        int resumeTalkPageIndex,
        std::size_t sourceTalkPageIndex);
    void Finish();
    void Reset();
    void PreparePlayingScene() { mIsFinishing = false; }
    void PrepareStageChange() { ClearResume(); }
    bool HasResume() const { return mHasResume; }

private:
    void ClearResume();

    Game& mGame;
    SceneSystem& mSceneSystem;
    UIState& mUIState;
    TalkController& mTalkController;
    std::function<void()> mSuppressForcedArrivalTalk;

    bool mHasResume = false;
    bool mIsFinishing = false;
    int mResumeTalkPageIndex = -1;
    int mReturnStageNum = -1;
    int mResumePlayerIndex = 0;
    std::string mReturnStageYamlPath;
    std::string mResumeNPCConversationId;
};
