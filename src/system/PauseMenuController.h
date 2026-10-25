#pragma once

class Game;

// ポーズメニューの開閉・選択状態を保持し、選択されたゲーム操作を実行する。
class PauseMenuController {
public:
    // 開閉を切り替える。開く場合は選択位置を先頭へ戻す。
    void Toggle();
    void Close();

    // deltaに-1か1を指定して巡回し、選択できない項目を飛ばす。
    void MoveSelection(const Game& game, int delta);

    // 現在の項目に対応するゲーム操作を実行する。無効な項目では何もしない。
    void ExecuteSelectedItem(Game& game);

    bool IsOpen() const { return mIsOpen; }
    int GetSelectedIndex() const { return mSelectedIndex; }

private:
    static bool IsItemEnabled(const Game& game, int index);

    bool mIsOpen = false;
    int mSelectedIndex = 0;
};
