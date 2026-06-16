#pragma once

#include <string>

class Game;

class UIState {
public:
    enum class TalkWith { None, Opening, Mother, Doctor, NPC };

    enum class TutorialKind { None, Battle, Break, Jewel, JustDodge };

    UIState(Game* game);
    void IncTalkUIIndex() { mTalkUIIndex++; }
    void StartTalkWith(TalkWith talkWith);
    void FinishTutorial();
    void FinishTalkWith();
    void OnFadeIn();

    void SetIsBattleTutorialShown(bool isBattleTutorialShown) { mIsBattleTutorialShown = isBattleTutorialShown; }
    void SetIsJustDodgeTutorialShown(bool isJustDodgeTutorialShown)
    {
        mIsJustDodgeTutorialShown = isJustDodgeTutorialShown;
    }
    void SetIsSpecialAttackTutorialShown(bool isSpecialAttackTutorialShown)
    {
        mIsSpecialAttackTutorialShown = isSpecialAttackTutorialShown;
    }
    void SetIsOperationUIShow(bool isOperationUIShow) { mIsOperationUIShow = isOperationUIShow; }

    void SetTalkUIIndex(int talkUIIndex) { mTalkUIIndex = talkUIIndex; }

    void SetCurrentTalkWith(TalkWith currentTalkWith) { mCurrentTalkWith = currentTalkWith; }
    void SetCurrentTutorialKind(TutorialKind currentTutorialKind) { mCurrentTutorialKind = currentTutorialKind; }

    bool GetIsBattleTutorialShown() const { return mIsBattleTutorialShown; }
    bool GetIsJustDodgeTutorialShown() const { return mIsJustDodgeTutorialShown; }
    bool GetIsSpecialAttackTutorialShown() const { return mIsSpecialAttackTutorialShown; }
    bool GetIsOperationUIShow() const { return mIsOperationUIShow; }

    int GetTalkUIIndex() const { return mTalkUIIndex; }

    TalkWith GetCurrentTalkWith() const { return mCurrentTalkWith; }
    TutorialKind GetCurrentTutorialKind() const { return mCurrentTutorialKind; }

private:
    bool mIsBattleTutorialShown;
    bool mIsJustDodgeTutorialShown;
    bool mIsSpecialAttackTutorialShown;
    bool mIsOperationUIShow;

    int mTalkUIIndex;

    TalkWith mCurrentTalkWith;
    TutorialKind mCurrentTutorialKind;

    Game* mGame;
};