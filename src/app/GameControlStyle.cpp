#include "actor/Player.h"
#include "app/Game.h"

#include "system/GameProgressController.h"

void Game::TogglePlayerControlStyle()
{
    SetPlayerControlStyle(IsAssistControlStyle() ? PlayerControlStyle::Standard : PlayerControlStyle::Assist);
}

void Game::SetPlayerControlStyle(PlayerControlStyle controlStyle)
{
    if (mPlayerControlStyle != controlStyle) {
        for (Player* player : GetPlayers()) {
            if (player) {
                player->SuppressCombatInputUntilReleased();
            }
        }
    }
    mPlayerControlStyle = controlStyle;
    if (mProgressController) {
        mProgressController->SetSelectedPlayerControlStyle(controlStyle);
    }
}

bool Game::HasSelectedPlayerControlStyle() const
{
    return mProgressController && mProgressController->HasSelectedPlayerControlStyle();
}
