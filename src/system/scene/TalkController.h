#pragma once

#include <cstdint>

class Game;
class GameProgressState;
class UIState;
class NPC;
class Player;
enum class TalkPageAdvanceCondition;

// NPC会話の開始・再開・ページ進行を管理し、決定入力とPlayerの行動条件を処理する。
class TalkController {
public:
    TalkController(Game* game, GameProgressState* gameProgressState, UIState* uiState, NPC*& talkingNPC,
                   Player*& talkingPlayer);

    void Update(float deltaTime);
    void TryAdvanceTalkFromConfirm();

    // NPCとPlayerを借用して会話を開始し、UI・シーン状態を切り替える。
    void StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer);

    // 0始まりのページから会話を再開する。NPC・Playerは借用する。
    void ResumeTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer, int talkPageIndex);

    // Stage再読込前に呼び、会話と行動待ちが保持するActor参照を解除する。
    void ClearActorReferencesForStageReload();

    // 1始まりのPlayer番号で会話可能なNPCを探し、会話を開始できればtrueを返す。
    bool TryStartTalkWithNPC(int playerNum);
    bool CanStartTalkWithNPC(const Player* player) const;

    bool IsWaitingForPlayerAction() const;
    bool IsWaitingForPlayerSwitch() const;
    bool IsWaitingForPlayerJump() const;

private:
    bool FinishTalkIfComplete();
    TalkPageAdvanceCondition GetCurrentAdvanceCondition() const;
    void CaptureCurrentPageActionBaseline();
    bool TryAdvanceTalkFromCompletedAction();
    void AdvanceTalkPage();

    Game* mGame;
    GameProgressState* mGameProgressState;
    UIState* mUIState;

    NPC*& mTalkingNPC;
    Player*& mTalkingPlayer;

    Player* mActionPlayerAtPageStart = nullptr;
    int mControlledPlayerIndexAtPageStart = -1;
    std::uint64_t mJumpSequenceAtPageStart = 0;
    bool mHasJumpStartedOnCurrentPage = false;
};
