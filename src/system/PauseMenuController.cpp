#include "system/PauseMenuController.h"

#include "Game.h"

void PauseMenuController::Toggle()
{
    mIsOpen = !mIsOpen;

    if (mIsOpen) {
        mSelectedIndex = 0;
    }
}

void PauseMenuController::Close()
{
    mIsOpen = false;
}

void PauseMenuController::MoveSelection(const Game& game, int delta)
{
    constexpr int menuItemCount = 6;


    for (int attempt = 0; attempt < menuItemCount; ++attempt) {
        mSelectedIndex =
            (mSelectedIndex + delta + menuItemCount) % menuItemCount;
        if (IsItemEnabled(game, mSelectedIndex)) {
            return;
        }
    }
}

void PauseMenuController::ExecuteSelectedItem(Game& game)
{
    if (!IsItemEnabled(game, mSelectedIndex)) {
        return;
    }

    switch (mSelectedIndex) {
    case 0:
        Close();
        break;

    case 1:
        game.TogglePlayerControlStyle();
        break;

    case 2:
        if (game.GetIsPlayer2Joined()) {
            game.ReturnToSinglePlayer();
        } else {
            game.TryCreatePlayer2();
        }
        break;

    case 3:
        game.ReturnToBase();
        break;

    case 4:
        game.OpenFeedbackForm();
        break;

    case 5:
        game.FinishGame();
        break;

    default:
        break;
    }
}

bool PauseMenuController::IsItemEnabled(const Game& game, int index)
{
    switch (index) {
    case 2:
        return game.CanStartTwoPlayerFromPauseMenu();
    case 3:
        return game.CanReturnToBaseFromPauseMenu();
    default:
        return true;
    }
}
