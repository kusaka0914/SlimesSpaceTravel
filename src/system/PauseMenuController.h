#pragma once

class Game;

class PauseMenuController {
public:
    void Toggle();
    void Close();
    void MoveSelection(int delta);
    void ExecuteSelectedItem(Game& game);

    bool IsOpen() const { return mIsOpen; }
    int GetSelectedIndex() const { return mSelectedIndex; }

private:
    bool mIsOpen = false;
    int mSelectedIndex = 0;
};
