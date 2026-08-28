#include "TestSupport.h"

#include "gfx/debug/ugc/UGCEditorTutorial.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void ProgressStartsEmptyAndAdvancesToFirstAction()
{
    UGCEditorTutorial tutorial;
    tutorial.Start();

    ExpectEqual(0, tutorial.GetCurrentActionNumber(), "welcome action number");
    ExpectEqual(14, tutorial.GetActionCount(), "action count");
    ExpectEqual(0.0f, tutorial.GetProgressRatio(), "welcome progress");

    tutorial.AdvanceFromWelcome();
    ExpectEqual(1, tutorial.GetCurrentActionNumber(), "first action number");
}

void ExpectedActionsAdvanceThroughTutorial()
{
    UGCEditorTutorial tutorial;
    tutorial.Start();
    tutorial.AdvanceFromWelcome();
    tutorial.RecordViewAdjustment();
    tutorial.RecordLayerChange(1, false);
    tutorial.RecordLayerChange(-1, false);
    tutorial.RecordPlacement(UGCPresetKind::NormalPlatform, 1);
    tutorial.RecordPlacement(UGCPresetKind::NormalPlatform, 2);
    tutorial.RecordSelectionMove();
    tutorial.RecordLayerChange(1, true);
    tutorial.RecordLayerChange(-1, true);
    tutorial.RecordUndo(true);
    tutorial.RecordErase(true);
    tutorial.RecordPlacement(UGCPresetKind::NormalEnemy, 1);
    tutorial.RecordPlacement(UGCPresetKind::GoalStar, 1);
    tutorial.RecordPlaytestStarted();
    tutorial.RecordReturnedFromPlaytest();

    ExpectEqual(
        static_cast<int>(UGCEditorTutorialStep::Complete),
        static_cast<int>(tutorial.GetStep()),
        "completed tutorial step");
}

void UnrelatedPlacementDoesNotAdvancePlatformStep()
{
    UGCEditorTutorial tutorial;
    tutorial.Start();
    tutorial.AdvanceFromWelcome();
    tutorial.RecordViewAdjustment();
    tutorial.RecordLayerChange(1, false);
    tutorial.RecordLayerChange(-1, false);

    tutorial.RecordPlacement(UGCPresetKind::NormalEnemy, 1);

    ExpectEqual(
        static_cast<int>(UGCEditorTutorialStep::PlacePlatform),
        static_cast<int>(tutorial.GetStep()),
        "platform step after enemy placement");
}

void LargePlatformRequiresMultiCellFootprint()
{
    UGCEditorTutorial tutorial;
    tutorial.Start();
    tutorial.AdvanceFromWelcome();
    tutorial.RecordViewAdjustment();
    tutorial.RecordLayerChange(1, false);
    tutorial.RecordLayerChange(-1, false);
    tutorial.RecordPlacement(UGCPresetKind::NormalPlatform, 1);

    tutorial.RecordPlacement(UGCPresetKind::NormalPlatform, 1);

    ExpectEqual(
        static_cast<int>(UGCEditorTutorialStep::PlaceLargePlatform),
        static_cast<int>(tutorial.GetStep()),
        "large platform step after one-cell placement");
}

}

void RegisterUGCEditorTutorialTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCEditorTutorial.ProgressStartsEmptyAndAdvancesToFirstAction",
        ProgressStartsEmptyAndAdvancesToFirstAction);
    tests.emplace_back(
        "UGCEditorTutorial.ExpectedActionsAdvanceThroughTutorial",
        ExpectedActionsAdvanceThroughTutorial);
    tests.emplace_back(
        "UGCEditorTutorial.UnrelatedPlacementDoesNotAdvancePlatformStep",
        UnrelatedPlacementDoesNotAdvancePlatformStep);
    tests.emplace_back(
        "UGCEditorTutorial.LargePlatformRequiresMultiCellFootprint",
        LargePlatformRequiresMultiCellFootprint);
}
