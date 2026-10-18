#include "TestSupport.h"

#include "gfx/debug/stage/StageEditHistory.h"

#include <string>

namespace {

void NewEditClearsRedoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before first edit");
    history.CommitUndo("after first edit");

    history.PushUndoSnapshot("before second edit");

    ExpectEqual(std::size_t{0}, history.GetRedoCount(), "redo count");
    ExpectEqual(std::string("before second edit"), *history.FindUndoSnapshot(), "latest undo snapshot");
}

void UndoMovesCurrentSnapshotToRedoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before edit");

    history.CommitUndo("after edit");

    ExpectEqual(std::size_t{0}, history.GetUndoCount(), "undo count");
    ExpectEqual(std::size_t{1}, history.GetRedoCount(), "redo count");
    ExpectEqual(std::string("after edit"), *history.FindRedoSnapshot(), "redo snapshot");
}

void RedoMovesCurrentSnapshotBackToUndoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before edit");
    history.CommitUndo("after edit");

    history.CommitRedo("before edit");

    ExpectEqual(std::size_t{1}, history.GetUndoCount(), "undo count");
    ExpectEqual(std::size_t{0}, history.GetRedoCount(), "redo count");
    ExpectEqual(std::string("before edit"), *history.FindUndoSnapshot(), "undo snapshot after redo");
}

void UndoHistoryDiscardsOldestSnapshotAtLimit()
{
    StageEditHistory history(2);
    history.PushUndoSnapshot("first");
    history.PushUndoSnapshot("second");
    history.PushUndoSnapshot("third");

    history.CommitUndo("current");

    ExpectEqual(std::string("second"), *history.FindUndoSnapshot(), "oldest retained undo snapshot");
}

void EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps()
{
    StageEditHistory history;

    history.CommitUndo("current");
    history.CommitRedo("current");

    ExpectTrue(history.FindUndoSnapshot() == nullptr, "missing undo snapshot");
    ExpectTrue(history.FindRedoSnapshot() == nullptr, "missing redo snapshot");
}

void ClearRemovesUndoAndRedoSnapshots()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before edit");
    history.CommitUndo("after edit");
    history.PushUndoSnapshot("before next edit");

    history.Clear();

    ExpectEqual(std::size_t{0}, history.GetUndoCount(), "undo count");
    ExpectEqual(std::size_t{0}, history.GetRedoCount(), "redo count");
}

void SuccessfulStrokeSharesOneUndoAndPreservesFinalRedo()
{
    StageEditHistory history;
    history.RecordSavedChange("start", "cell1", StageEditGrouping::NewOperation);
    history.RecordSavedChange("cell1", "cell2", StageEditGrouping::ContinueOperation);
    history.RecordSavedChange("cell2", "final", StageEditGrouping::ContinueOperation);
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "stroke has one undo");
    ExpectEqual(std::string("start"), *history.FindUndoSnapshot(), "undo restores entire stroke");
    history.CommitUndo("final");
    ExpectEqual(std::string("final"), *history.FindRedoSnapshot(), "redo restores full stroke");
}

void ContinueWithoutSuccessfulPredecessorStartsNewUndo()
{
    StageEditHistory history;
    history.RecordSavedChange("start", "start", StageEditGrouping::NewOperation);
    history.RecordSavedChange("start", "changed", StageEditGrouping::ContinueOperation);
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "failed or unchanged first attempt does not consume undo");
    ExpectEqual(std::string("start"), *history.FindUndoSnapshot(), "successful retry has original baseline");
}

void InterveningEditAndUndoBreakStrokeGrouping()
{
    StageEditHistory history;
    history.RecordSavedChange("start", "first", StageEditGrouping::NewOperation);
    history.RecordSavedChange("external edit", "second", StageEditGrouping::ContinueOperation);
    ExpectEqual(std::size_t(2), history.GetUndoCount(), "different baseline cannot merge");
    history.CommitUndo("second");
    history.RecordSavedChange("external edit", "third", StageEditGrouping::ContinueOperation);
    ExpectEqual(std::size_t(2), history.GetUndoCount(), "undo breaks previous gesture");
    ExpectEqual(std::size_t(0), history.GetRedoCount(), "new successful edit clears redo");
}

void UnchangedOperationPreservesRedo()
{
    StageEditHistory history;
    history.PushUndoSnapshot("start");
    history.CommitUndo("edited");
    history.RecordSavedChange("start", "start", StageEditGrouping::NewOperation);
    ExpectEqual(std::size_t(0), history.GetUndoCount(), "no phantom undo");
    ExpectEqual(std::size_t(1), history.GetRedoCount(), "no-op preserves redo");
}

void StageBindingKeepsReloadHistoryAndRejectsCrossStageUndo()
{
    StageEditHistory history;
    history.BindToStage("stage1.yaml");
    history.RecordSavedChange("old stage1", "edited stage1", StageEditGrouping::NewOperation);
    history.BindToStage("stage1.yaml");
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "same-stage reload keeps undo");
    history.CommitUndo("edited stage1");
    history.BindToStage("stage1.yaml");
    ExpectEqual(std::size_t(1), history.GetRedoCount(), "same-stage reload keeps redo");
    history.BindToStage("stage2.yaml");
    ExpectEqual(std::size_t(0), history.GetUndoCount(), "other stage cannot receive old undo");
    ExpectEqual(std::size_t(0), history.GetRedoCount(), "other stage cannot receive old redo");
    history.BindToStage("stage1.yaml");
    ExpectEqual(std::size_t(0), history.GetRedoCount(), "old history does not reappear after returning");
}

} // namespace

void RegisterStageEditHistoryTests(TestRegistry& tests)
{
    tests.emplace_back("StageEditHistory.StageBinding", StageBindingKeepsReloadHistoryAndRejectsCrossStageUndo);
    tests.emplace_back("StageEditHistory.SuccessfulStrokeAndRedo", SuccessfulStrokeSharesOneUndoAndPreservesFinalRedo);
    tests.emplace_back("StageEditHistory.ContinuationAfterFailedStart",
                       ContinueWithoutSuccessfulPredecessorStartsNewUndo);
    tests.emplace_back("StageEditHistory.InterveningEditBreaksGrouping", InterveningEditAndUndoBreakStrokeGrouping);
    tests.emplace_back("StageEditHistory.NoOpPreservesRedo", UnchangedOperationPreservesRedo);
    tests.emplace_back("StageEditHistory.NewEditClearsRedoHistory", NewEditClearsRedoHistory);
    tests.emplace_back("StageEditHistory.UndoMovesCurrentSnapshotToRedoHistory", UndoMovesCurrentSnapshotToRedoHistory);
    tests.emplace_back("StageEditHistory.RedoMovesCurrentSnapshotBackToUndoHistory",
                       RedoMovesCurrentSnapshotBackToUndoHistory);
    tests.emplace_back("StageEditHistory.UndoHistoryDiscardsOldestSnapshotAtLimit",
                       UndoHistoryDiscardsOldestSnapshotAtLimit);
    tests.emplace_back("StageEditHistory.EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps",
                       EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps);
    tests.emplace_back("StageEditHistory.ClearRemovesUndoAndRedoSnapshots", ClearRemovesUndoAndRedoSnapshots);
}
