#include "gfx/debug/ugc/UGCPreviewState.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr int minimumPreviewWidth = 320;
constexpr int minimumPreviewHeight = 180;
constexpr int maximumPreviewWidth = 2048;
constexpr int maximumPreviewHeight = 1152;
constexpr float piRadians = 3.14159265359f;
constexpr float fullTurnRadians = 6.28318530718f;
constexpr float focusSmoothingRatePerSecond = 10.0f;
}

void UGCPreviewState::SetRequestedRenderSize(int width, int height)
{
    mRequestedRenderWidth = std::clamp(
        width, minimumPreviewWidth, maximumPreviewWidth);
    mRequestedRenderHeight = std::clamp(
        height, minimumPreviewHeight, maximumPreviewHeight);
}

int UGCPreviewState::GetRequestedRenderWidth() const
{
    return mRequestedRenderWidth;
}

int UGCPreviewState::GetRequestedRenderHeight() const
{
    return mRequestedRenderHeight;
}

void UGCPreviewState::AdjustYawRadians(float yawDeltaRadians)
{
    mYawRadians += yawDeltaRadians;
    while (mYawRadians > piRadians) {
        mYawRadians -= fullTurnRadians;
    }
    while (mYawRadians < -piRadians) {
        mYawRadians += fullTurnRadians;
    }
}

float UGCPreviewState::GetYawRadians() const
{
    return mYawRadians;
}

void UGCPreviewState::ToggleVerticalView()
{
    mIsViewedFromBelow = !mIsViewedFromBelow;
}

bool UGCPreviewState::IsViewedFromBelow() const
{
    return mIsViewedFromBelow;
}

void UGCPreviewState::SetEditLayer(int layer)
{
    mEditLayer = layer;
}

int UGCPreviewState::GetEditLayer() const
{
    return mEditLayer;
}

float UGCPreviewState::UpdateFocusY(float gridSize, float deltaSeconds)
{
    const float targetFocusY = static_cast<float>(mEditLayer) * gridSize;
    if (!mHasFocusY) {
        mFocusY = targetFocusY;
        mHasFocusY = true;
        return mFocusY;
    }

    const float smoothing = 1.0f - std::exp(
        -focusSmoothingRatePerSecond * std::max(0.0f, deltaSeconds));
    mFocusY += (targetFocusY - mFocusY) * smoothing;
    return mFocusY;
}

float UGCPreviewState::GetFocusY() const
{
    return mFocusY;
}
