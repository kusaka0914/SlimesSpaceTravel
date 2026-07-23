#include "Game.h"

void Game::TogglePlayerControlStyle()
{
    mPlayerControlStyle = IsAssistControlStyle()
                              ? PlayerControlStyle::Standard
                              : PlayerControlStyle::Assist;
}
