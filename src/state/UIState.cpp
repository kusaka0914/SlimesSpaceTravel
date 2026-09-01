#include "UIState.h"

UIState::UIState()
    : mIsOperationUIShow(true),
      mTalkUIIndex(0),
      mCurrentTalkWith(TalkWith::Opening)
{
}

void UIState::StartTalkWith(TalkWith talkWith)
{
    mTalkUIIndex = 0;
    mCurrentTalkWith = talkWith;
}

void UIState::FinishTutorial()
{
    mTalkUIIndex = 0;
}

void UIState::FinishTalkWith()
{
    mTalkUIIndex = 0;
    mCurrentTalkWith = TalkWith::None;
}

void UIState::OnFadeIn()
{
    mTalkUIIndex = -1;
}
