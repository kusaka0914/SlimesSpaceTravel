#include "gfx/debug/ugc/UGCPreviewPanelState.h"

#include <algorithm>

void UGCPreviewPanelState::InitializeWidth(
    float preferredWidth,
    float minimumWidth,
    float maximumWidth)
{
    if (mHasInitializedWidth) {
        return;
    }
    mWidth = std::clamp(
        preferredWidth,
        minimumWidth,
        maximumWidth);
    mResizeStartWidth = mWidth;
    mHasInitializedWidth = true;
}

void UGCPreviewPanelState::BeginResize()
{
    mResizeStartWidth = mWidth;
}

void UGCPreviewPanelState::Resize(
    float horizontalMouseDelta,
    float minimumWidth,
    float maximumWidth)
{
    mWidth = std::clamp(
        mResizeStartWidth - horizontalMouseDelta,
        minimumWidth,
        maximumWidth);
}

void UGCPreviewPanelState::SetWidth(
    float width,
    float minimumWidth,
    float maximumWidth)
{
    mWidth = std::clamp(width, minimumWidth, maximumWidth);
}

float UGCPreviewPanelState::GetWidth() const
{
    return mWidth;
}

float UGCPreviewPanelState::GetResizeStartWidth() const
{
    return mResizeStartWidth;
}

bool UGCPreviewPanelState::HasInitializedWidth() const
{
    return mHasInitializedWidth;
}
