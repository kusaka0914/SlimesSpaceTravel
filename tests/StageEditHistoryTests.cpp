#include "TestSupport.h"

#include "gfx/debug/stage/StageEditHistory.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void NewEditClearsRedoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before first edit");
    history.CommitUndo("after first edit");

    history.PushUndoSnapshot("before second edit");

    ExpectEqual(std::size_t{0}, history.GetRedoCount(), "redo count");
    ExpectEqual(
        std::string("before second edit"),
        *history.FindUndoSnapshot(),
        "latest undo snapshot");
}

void UndoMovesCurrentSnapshotToRedoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before edit");

    history.CommitUndo("after edit");

    ExpectEqual(std::size_t{0}, history.GetUndoCount(), "undo count");
    ExpectEqual(std::size_t{1}, history.GetRedoCount(), "redo count");
    ExpectEqual(
        std::string("after edit"),
        *history.FindRedoSnapshot(),
        "redo snapshot");
}

void RedoMovesCurrentSnapshotBackToUndoHistory()
{
    StageEditHistory history;
    history.PushUndoSnapshot("before edit");
    history.CommitUndo("after edit");

    history.CommitRedo("before edit");

    ExpectEqual(std::size_t{1}, history.GetUndoCount(), "undo count");
    ExpectEqual(std::size_t{0}, history.GetRedoCount(), "redo count");
    ExpectEqual(
        std::string("before edit"),
        *history.FindUndoSnapshot(),
        "undo snapshot after redo");
}

void UndoHistoryDiscardsOldestSnapshotAtLimit()
{
    StageEditHistory history(2);
    history.PushUndoSnapshot("first");
    history.PushUndoSnapshot("second");
    history.PushUndoSnapshot("third");

    history.CommitUndo("current");

    ExpectEqual(
        std::string("second"),
        *history.FindUndoSnapshot(),
        "oldest retained undo snapshot");
}

void EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps()
{
    StageEditHistory history;

    history.CommitUndo("current");
    history.CommitRedo("current");

    ExpectTrue(history.FindUndoSnapshot() == nullptr, "missing undo snapshot");
    ExpectTrue(history.FindRedoSnapshot() == nullptr, "missing redo snapshot");
}

}

void RegisterStageEditHistoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back("StageEditHistory.NewEditClearsRedoHistory", NewEditClearsRedoHistory);
    tests.emplace_back("StageEditHistory.UndoMovesCurrentSnapshotToRedoHistory", UndoMovesCurrentSnapshotToRedoHistory);
    tests.emplace_back("StageEditHistory.RedoMovesCurrentSnapshotBackToUndoHistory", RedoMovesCurrentSnapshotBackToUndoHistory);
    tests.emplace_back("StageEditHistory.UndoHistoryDiscardsOldestSnapshotAtLimit", UndoHistoryDiscardsOldestSnapshotAtLimit);
    tests.emplace_back("StageEditHistory.EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps", EmptyHistoryHasNoSnapshotsAndCommitsAreNoOps);
}
