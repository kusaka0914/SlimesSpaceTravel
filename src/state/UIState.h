#pragma once

#include <string>

class Game;

class UIState {
public:
    enum class TalkWith { None, Opening, Mother, Doctor, NPC };

    UIState(Game* game);
    void IncTalkUIIndex() { mTalkUIIndex++; }
    void StartTalkWith(TalkWith talkWith);
    void FinishTutorial();
    void FinishTalkWith();
    void OnFadeIn();

    void SetIsOperationUIShow(bool isOperationUIShow) { mIsOperationUIShow = isOperationUIShow; }

    void SetTalkUIIndex(int talkUIIndex) { mTalkUIIndex = talkUIIndex; }

    void SetCurrentTalkWith(TalkWith currentTalkWith) { mCurrentTalkWith = currentTalkWith; }

    bool GetIsOperationUIShow() const { return mIsOperationUIShow; }

    int GetTalkUIIndex() const { return mTalkUIIndex; }

    TalkWith GetCurrentTalkWith() const { return mCurrentTalkWith; }

private:
    bool mIsOperationUIShow;

    int mTalkUIIndex;

    TalkWith mCurrentTalkWith;
    Game* mGame;
};
