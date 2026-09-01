#include "Game.h"

#include "system/GameProgressController.h"

void Game::TogglePlayerControlStyle()
{
    SetPlayerControlStyle(
        IsAssistControlStyle()
            ? PlayerControlStyle::Standard
            : PlayerControlStyle::Assist);
}

void Game::SetPlayerControlStyle(PlayerControlStyle controlStyle)
{
    mPlayerControlStyle = controlStyle;
    if (mProgressController) {
        mProgressController->SetSelectedPlayerControlStyle(controlStyle);
    }
}

bool Game::HasSelectedPlayerControlStyle() const
{
    return mProgressController &&
           mProgressController->HasSelectedPlayerControlStyle();
}
