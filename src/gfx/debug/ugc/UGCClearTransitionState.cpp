#include "gfx/debug/ugc/UGCClearTransitionState.h"

bool UGCClearTransitionState::IsTransitionInProgress() const
{
    return mIsTransitionInProgress;
}

bool UGCClearTransitionState::HasPendingCompletion() const
{
    return mPendingWorkFileName.has_value();
}

void UGCClearTransitionState::QueueCompletion(
    const std::string& workFileName,
    UGCClearDestination destination)
{
    if (!mIsTransitionInProgress && !mPendingWorkFileName) {
        mPendingWorkFileName = workFileName;
        mDestination = destination;
    }
}

UGCClearDestination UGCClearTransitionState::GetDestination() const
{
    return mDestination;
}

const std::string& UGCClearTransitionState::GetCompletedWorkFileName() const
{
    static const std::string emptyWorkFileName;
    return mPendingWorkFileName
        ? *mPendingWorkFileName
        : emptyWorkFileName;
}

void UGCClearTransitionState::BeginTransition()
{
    if (!mPendingWorkFileName) {
        return;
    }

    mPendingWorkFileName.reset();
    mIsTransitionInProgress = true;
}

void UGCClearTransitionState::CompleteTransition()
{
    mPendingWorkFileName.reset();
    mIsTransitionInProgress = false;
}
