#include "UIState.h"
#include "Game.h"

UIState::UIState(Game* game)
    : mIsOperationUIShow(true),
      mTalkUIIndex(0),
      mCurrentTalkWith(TalkWith::Opening),
      mGame(game)
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
