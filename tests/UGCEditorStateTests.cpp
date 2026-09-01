#include "TestSupport.h"

#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCPreviewPanelState.h"
#include "gfx/debug/ugc/UGCSelectionDragState.h"
#include "gfx/debug/ugc/UGCSwitchConnectionState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void EraserRestoresThePresetThatWasActiveBeforeIt()
{
    UGCEditorToolState state;
    state.ActivatePreset(UGCPresetKind::MovingPlatform);

    state.EnterEraser(state.activePresetKind);
    ExpectTrue(state.isEraserMode, "eraser is active");
    ExpectFalse(state.activePresetKind.has_value(), "placement is inactive");

    state.LeaveEraser(true);
    ExpectFalse(state.isEraserMode, "eraser is inactive");
    ExpectTrue(state.activePresetKind.has_value(), "preset was restored");
    ExpectTrue(
        *state.activePresetKind == UGCPresetKind::MovingPlatform,
        "restored preset");
}

void LoadedWorkResetsOnlyTransientToolState()
{
    UGCEditorToolState state;
    state.ActivatePreset(UGCPresetKind::NormalEnemy);
    state.statusMessage = "old status";
    state.editLayer = 4;
    state.platformFootprintSideLength = 3;

    state.ResetForLoadedWork();

    ExpectFalse(state.isEraserMode, "eraser reset");
    ExpectFalse(state.activePresetKind.has_value(), "preset reset");
    ExpectEqual(0, state.editLayer, "edit layer reset");
    ExpectEqual(1, state.platformFootprintSideLength, "footprint reset");
    ExpectTrue(state.statusMessage.empty(), "status reset");
}

void DeactivatingEraserDoesNotRestoreThePreviousPreset()
{
    UGCEditorToolState state;
    state.ActivatePreset(UGCPresetKind::MovingPlatform);
    state.EnterEraser(state.activePresetKind);

    state.DeactivateEraser();

    ExpectFalse(state.isEraserMode, "eraser is inactive");
    ExpectFalse(state.activePresetKind.has_value(), "preset remains inactive");
    ExpectFalse(
        state.presetBeforeEraser.has_value(),
        "preset restoration request is cleared");
}

void MenuRequestsAreConsumedOnlyOnce()
{
    UGCEditorMenuState state;
    state.RequestMenuOpen();
    state.RequestWorkManagementOpen();

    ExpectTrue(state.ConsumeMenuOpenRequest(), "first menu request");
    ExpectFalse(state.ConsumeMenuOpenRequest(), "second menu request");
    ExpectTrue(
        state.ConsumeWorkManagementOpenRequest(),
        "first work management request");
    ExpectFalse(
        state.ConsumeWorkManagementOpenRequest(),
        "second work management request");
}

void DragResetClearsAllTransientMovement()
{
    UGCSelectionDragState state;
    state.isDragging = true;
    state.hasMoved = true;
    state.appliedDelta = glm::vec3(1.0f, 2.0f, 3.0f);
    state.actorRefs.push_back(
        StageActorRef{StageActorType::Platform, 2, "platforms", "platform"});

    state.Reset();

    ExpectFalse(state.isDragging, "dragging reset");
    ExpectFalse(state.hasMoved, "movement reset");
    ExpectTrue(state.actorRefs.empty(), "actor references reset");
    ExpectNear(0.0f, state.appliedDelta.x, 0.0001f, "x delta reset");
}

void SwitchConnectionCancelKeepsNoPendingTarget()
{
    UGCSwitchConnectionState state;
    const StageActorRef switchRef{
        StageActorType::Platform, 3, "platforms", "switch"};
    state.Begin(switchRef, UGCSwitchConnectionAction::Disconnect);

    ExpectTrue(state.HasPendingConnection(), "connection is pending");
    ExpectTrue(
        state.GetAction() == UGCSwitchConnectionAction::Disconnect,
        "disconnect action");

    state.Cancel();
    ExpectFalse(state.HasPendingConnection(), "connection cancelled");
}

void PreviewWidthInitializationAndResizeStayWithinTheViewport()
{
    UGCPreviewPanelState state;
    state.InitializeWidth(320.0f, 180.0f, 500.0f);
    ExpectNear(320.0f, state.GetWidth(), 0.0001f, "initial width");

    state.BeginResize();
    state.Resize(-1000.0f, 180.0f, 468.0f);
    ExpectNear(468.0f, state.GetWidth(), 0.0001f, "viewport maximum");
}

}

void RegisterUGCEditorStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCEditorToolState.EraserRestoresThePresetThatWasActiveBeforeIt",
        EraserRestoresThePresetThatWasActiveBeforeIt);
    tests.emplace_back(
        "UGCEditorToolState.LoadedWorkResetsOnlyTransientToolState",
        LoadedWorkResetsOnlyTransientToolState);
    tests.emplace_back(
        "UGCEditorToolState.DeactivatingEraserDoesNotRestoreThePreviousPreset",
        DeactivatingEraserDoesNotRestoreThePreviousPreset);
    tests.emplace_back(
        "UGCEditorMenuState.MenuRequestsAreConsumedOnlyOnce",
        MenuRequestsAreConsumedOnlyOnce);
    tests.emplace_back(
        "UGCSelectionDragState.DragResetClearsAllTransientMovement",
        DragResetClearsAllTransientMovement);
    tests.emplace_back(
        "UGCSwitchConnectionState.SwitchConnectionCancelKeepsNoPendingTarget",
        SwitchConnectionCancelKeepsNoPendingTarget);
    tests.emplace_back(
        "UGCPreviewPanelState.PreviewWidthInitializationAndResizeStayWithinTheViewport",
        PreviewWidthInitializationAndResizeStayWithinTheViewport);
}
