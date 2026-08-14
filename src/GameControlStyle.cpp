#include "Game.h"

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
}
