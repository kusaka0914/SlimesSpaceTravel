#include "gfx/debug/ugc/UGCSessionState.h"

#include <utility>

void UGCSessionState::EnterEditor()
{
    mVerificationWorkFileName.clear();
    mIsClearCompletionPending = false;
    mIsWorkBrowserShowing = false;
    mIsModeActive = true;
    mIsPlaytestActive = false;
    mIsDebugPanelShowing = false;
    mIsOrthographicView = true;
    mIsClearResultShowing = false;
    mClearResultSelection = 0;
}

void UGCSessionState::ToggleDebugPanel()
{
    if (mIsModeActive) {
        mIsDebugPanelShowing = !mIsDebugPanelShowing;
    }
}

bool UGCSessionState::StartPlaytest(UGCPlaytestPurpose purpose)
{
    if (!mIsModeActive) {
        return false;
    }
    mIsPlaytestActive = true;
    mIsDebugPanelShowing = false;
    mIsOrthographicView = false;
    mIsClearResultShowing = false;
    mClearResultSelection = 0;
    mPlaytestPurpose = purpose;
    return true;
}

bool UGCSessionState::StartVerification(
    const std::string& workFileName)
{
    if (!mIsModeActive || workFileName.empty()) {
        return false;
    }
    mVerificationWorkFileName = workFileName;
    mIsClearCompletionPending = false;
    return true;
}

bool UGCSessionState::ReturnToEditor()
{
    if (!mIsModeActive) {
        return false;
    }
    mVerificationWorkFileName.clear();
    mIsClearCompletionPending = false;
    mIsPlaytestActive = false;
    mIsDebugPanelShowing = false;
    mIsOrthographicView = true;
    mIsClearResultShowing = false;
    mClearResultSelection = 0;
    return true;
}

void UGCSessionState::Exit()
{
    mVerificationWorkFileName.clear();
    mIsModeActive = false;
    mIsPlaytestActive = false;
    mIsDebugPanelShowing = false;
    mIsClearCompletionPending = false;
    mIsWorkBrowserShowing = false;
    mIsOrthographicView = false;
    mIsClearResultShowing = false;
    mClearResultSelection = 0;
}

void UGCSessionState::OpenWorkBrowser()
{
    mIsWorkBrowserShowing = true;
}

void UGCSessionState::CloseWorkBrowser()
{
    mIsWorkBrowserShowing = false;
}

void UGCSessionState::MarkClearCompletionPending()
{
    if (mIsModeActive) {
        mIsClearCompletionPending = true;
    }
}

std::optional<std::string> UGCSessionState::ConsumeClearCompletion()
{
    if (!mIsClearCompletionPending) {
        return std::nullopt;
    }

    mIsClearCompletionPending = false;
    if (!mIsModeActive) {
        return std::nullopt;
    }

    std::string workFileName = std::move(mVerificationWorkFileName);
    mVerificationWorkFileName.clear();
    return workFileName;
}

void UGCSessionState::ShowClearResult()
{
    if (!mIsModeActive || !mIsPlaytestActive) {
        return;
    }
    mIsClearResultShowing = true;
    mClearResultSelection = 0;
}

void UGCSessionState::MoveClearResultSelection(int delta, int itemCount)
{
    if (!mIsClearResultShowing || delta == 0 || itemCount <= 0) {
        return;
    }
    mClearResultSelection =
        (mClearResultSelection + delta + itemCount) % itemCount;
}

bool UGCSessionState::IsModeActive() const
{
    return mIsModeActive;
}

bool UGCSessionState::IsPlaytestActive() const
{
    return mIsPlaytestActive;
}

bool UGCSessionState::IsVerificationActive() const
{
    return mIsModeActive && mIsPlaytestActive &&
        !mVerificationWorkFileName.empty();
}

bool UGCSessionState::IsClearResultShowing() const
{
    return mIsClearResultShowing;
}

int UGCSessionState::GetClearResultSelection() const
{
    return mClearResultSelection;
}

UGCPlaytestPurpose UGCSessionState::GetPlaytestPurpose() const
{
    return mPlaytestPurpose;
}

bool UGCSessionState::IsDebugPanelShowing() const
{
    return mIsDebugPanelShowing;
}

bool UGCSessionState::IsWorkBrowserShowing() const
{
    return mIsWorkBrowserShowing;
}

bool UGCSessionState::IsOrthographicView() const
{
    return mIsOrthographicView;
}

void UGCSessionState::SetOrthographicView(bool isOrthographicView)
{
    mIsOrthographicView = isOrthographicView;
}
