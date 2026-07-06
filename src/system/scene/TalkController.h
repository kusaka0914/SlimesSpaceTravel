#pragma once

class Game;
class GameProgressState;
class UIState;
class NPC;
class Player;

class TalkController {
public:
    TalkController(Game* game, GameProgressState* gameProgressState, UIState* uiState, NPC*& talkingNPC,
                   Player*& talkingPlayer);

    void AdvanceTalk();
    void StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer);
    void TryStartTalkWithNPC(int playerNum);

private:
    Game* mGame;
    GameProgressState* mGameProgressState;
    UIState* mUIState;

    NPC*& mTalkingNPC;
    Player*& mTalkingPlayer;
};
