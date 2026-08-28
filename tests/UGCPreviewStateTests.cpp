#include "TestSupport.h"

#include "gfx/debug/ugc/UGCPreviewState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void RequestedRenderSizeIsClampedToSupportedRange()
{
    UGCPreviewState preview;

    preview.SetRequestedRenderSize(1, 5000);

    ExpectEqual(320, preview.GetRequestedRenderWidth(), "minimum width");
    ExpectEqual(1152, preview.GetRequestedRenderHeight(), "maximum height");
}

void YawIsKeptWithinOneHalfTurn()
{
    UGCPreviewState preview;

    preview.AdjustYawRadians(6.28318530718f + 0.25f);

    ExpectNear(0.25f, preview.GetYawRadians(), 0.0001f, "normalized yaw");
}

void FocusStartsAtCurrentLayerAndThenSmoothlyFollowsIt()
{
    UGCPreviewState preview;
    preview.SetEditLayer(2);

    ExpectNear(
        3.0f,
        preview.UpdateFocusY(1.5f, 0.0f),
        0.0001f,
        "initial focus");

    preview.SetEditLayer(4);
    const float updatedFocus = preview.UpdateFocusY(1.5f, 0.1f);

    ExpectTrue(updatedFocus > 3.0f, "focus moves toward new layer");
    ExpectTrue(updatedFocus < 6.0f, "focus does not jump after initialization");
}

void VerticalViewToggleChangesDirection()
{
    UGCPreviewState preview;

    preview.ToggleVerticalView();

    ExpectTrue(preview.IsViewedFromBelow(), "view direction after first toggle");
    preview.ToggleVerticalView();
    ExpectFalse(preview.IsViewedFromBelow(), "view direction after second toggle");
}

}

void RegisterUGCPreviewStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCPreviewState.RequestedRenderSizeIsClampedToSupportedRange",
        RequestedRenderSizeIsClampedToSupportedRange);
    tests.emplace_back(
        "UGCPreviewState.YawIsKeptWithinOneHalfTurn",
        YawIsKeptWithinOneHalfTurn);
    tests.emplace_back(
        "UGCPreviewState.FocusStartsAtCurrentLayerAndThenSmoothlyFollowsIt",
        FocusStartsAtCurrentLayerAndThenSmoothlyFollowsIt);
    tests.emplace_back(
        "UGCPreviewState.VerticalViewToggleChangesDirection",
        VerticalViewToggleChangesDirection);
}
