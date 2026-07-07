#include "system/scene/TalkController.h"

#include "Game.h"
#include "actor/NPC.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"

#include <vector>

TalkController::TalkController(Game* game, GameProgressState* gameProgressState, UIState* uiState, NPC*& talkingNPC,
                               Player*& talkingPlayer)
    : mGame(game),
      mGameProgressState(gameProgressState),
      mUIState(uiState),
      mTalkingNPC(talkingNPC),
      mTalkingPlayer(talkingPlayer)
{
}

void TalkController::AdvanceTalk()
{
    mUIState->IncTalkUIIndex();
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void TalkController::StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer)
{
    if (!talkingNPC || !talkingPlayer) {
        return;
    }

    mTalkingNPC = talkingNPC;
    mTalkingPlayer = talkingPlayer;

    mUIState->SetCurrentTalkWith(UIState::TalkWith::NPC);
    mUIState->SetTalkUIIndex(0);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void TalkController::TryStartTalkWithNPC(int playerNum)
{
    const std::vector<Player*>& players = mGame->GetPlayers();

    for (Player* player : players) {
        if (!player) {
            continue;
        }

        if (player->GetPlayerNum() != playerNum) {
            continue;
        }

        NPC* talkableNPC = player->GetTalkableNPC();
        if (!talkableNPC) {
            return;
        }

        if (!talkableNPC->GetIsTalkable()) {
            return;
        }

        StartTalkWithNPC(talkableNPC, player);
        return;
    }
}
