#pragma once

class Game;

class PauseMenuController {
public:
    void Toggle();
    void Close();
    void MoveSelection(const Game& game, int delta);
    void ExecuteSelectedItem(Game& game);

    bool IsOpen() const { return mIsOpen; }
    int GetSelectedIndex() const { return mSelectedIndex; }

private:
    static bool IsItemEnabled(const Game& game, int index);

    bool mIsOpen = false;
    int mSelectedIndex = 0;
};
