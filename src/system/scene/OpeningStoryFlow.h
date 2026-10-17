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

// 会話から物語演出へ移り、Stage・NPC・ページを識別情報から解決して会話を再開する。
class OpeningStoryFlow {
public:
    OpeningStoryFlow(Game& game, SceneSystem& sceneSystem, UIState& uiState, TalkController& talkController,
                     std::function<void()> suppressForcedArrivalTalk);

    // 0始まりの再開ページ・発火元ページを記録して物語演出を始める。既に実行済み・引数不正ならfalse。
    bool StartAfterTalkPage(NPC* talkingNPC, Player* talkingPlayer, int resumeTalkPageIndex,
                            std::size_t sourceTalkPageIndex);

    // 復帰先を識別情報から探して会話を再開するフェードを予約する。再開情報がなければ通常開始へ移る。
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
