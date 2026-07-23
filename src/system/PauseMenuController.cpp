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

void PauseMenuController::MoveSelection(int delta)
{
    constexpr int menuItemCount = 5;
    mSelectedIndex = (mSelectedIndex + delta + menuItemCount) % menuItemCount;
}

void PauseMenuController::ExecuteSelectedItem(Game& game)
{
    switch (mSelectedIndex) {
    case 0:
        Close();
        break;

    case 1:
        game.TogglePlayerControlStyle();
        break;

    case 2:
        game.ReturnToBase();
        break;

    case 3:
        game.OpenFeedbackForm();
        break;

    case 4:
        game.FinishGame();
        break;

    default:
        break;
    }
}
