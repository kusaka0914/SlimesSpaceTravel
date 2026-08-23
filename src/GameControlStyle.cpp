#include "Game.h"

#include "system/StageProgressSystem.h"

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
    if (mStageProgressSystem) {
        mStageProgressSystem->SetSelectedPlayerControlStyle(
            controlStyle == PlayerControlStyle::Assist);
    }
}

bool Game::HasSelectedPlayerControlStyle() const
{
    return mStageProgressSystem &&
           mStageProgressSystem->HasSelectedPlayerControlStyle();
}
