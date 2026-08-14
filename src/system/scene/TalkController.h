#pragma once

#include <cstdint>

class Game;
class GameProgressState;
class UIState;
class NPC;
class Player;
enum class TalkPageAdvanceCondition;

class TalkController {
public:
    TalkController(Game* game, GameProgressState* gameProgressState, UIState* uiState, NPC*& talkingNPC,
                   Player*& talkingPlayer);

    void Update(float deltaTime);
    void TryAdvanceTalkFromConfirm();
    void StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer);
    bool TryStartTalkWithNPC(int playerNum);
    bool CanStartTalkWithNPC(const Player* player) const;

    bool IsWaitingForPlayerAction() const;
    bool IsWaitingForPlayerSwitch() const;
    bool IsWaitingForPlayerJump() const;

private:
    TalkPageAdvanceCondition GetCurrentAdvanceCondition() const;
    void CaptureCurrentPageActionBaseline();
    bool TryAdvanceTalkFromCompletedAction();
    void AdvanceTalkPage();

private:
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
