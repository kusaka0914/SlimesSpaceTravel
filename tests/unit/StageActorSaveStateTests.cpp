#include "support/TestSupport.h"

#include "editor/stage/history/StageEditHistory.h"
#include "editor/stage/persistence/StageActorSaveState.h"

#include <vector>

namespace {
using Scope = StageActorSaveState::Scope;

void NoFailuresDoesNotInvokeSave()
{
    StageActorSaveState state;
    ExpectTrue(state.Retry("stage",
                           [](Scope, const auto&) {
                               throw std::runtime_error("unexpected retry");
                               return false;
                           }),
               "empty retry succeeds");
    ExpectTrue(state.GetError().empty(), "empty state has no error");
}

void RetryRetainsHistoryNotificationUntilSaveSucceeds()
{
    StageActorSaveState state;
    StageEditHistory history;
    history.PushUndoSnapshot("original");
    history.CommitUndo("edited");
    state.RecordFailure(Scope::AllActorStates, "stage", "write failed",
                        [&](const std::string& before) { history.PushUndoSnapshot(before); });

    ExpectFalse(state.Retry("stage", [](Scope, const auto&) { return false; }), "failed retry");
    ExpectEqual(std::size_t(0), history.GetUndoCount(), "failed save does not record undo");
    ExpectEqual(std::size_t(1), history.GetRedoCount(), "failed save preserves redo");
    ExpectEqual(std::string("write failed"), state.GetError(), "failure remains visible");

    ExpectTrue(state.Retry("stage",
                           [&](Scope scope, const auto& onSaved) {
                               state.ClearFailure(scope);
                               onSaved("before saved edit");
                               return true;
                           }),
               "successful retry");
    ExpectEqual(std::size_t(1), history.GetUndoCount(), "saved edit records exactly once");
    ExpectEqual(std::string("before saved edit"), *history.FindUndoSnapshot(), "pre-save bytes are retained");
    ExpectEqual(std::size_t(0), history.GetRedoCount(), "only successful save clears redo");
    ExpectTrue(state.GetError().empty(), "success clears error");
    ExpectTrue(state.Retry("stage", [](Scope, const auto&) { return false; }), "saved edit is not retried again");
}

void RetryOrdersScopesAndStopsAfterFailure()
{
    StageActorSaveState state;
    state.RecordFailure(Scope::AllActorStates, "stage", "settings failed", {});
    state.RecordFailure(Scope::AuthoredTransforms, "stage", "transform failed", {});
    std::vector<Scope> attempts;
    ExpectFalse(state.Retry("stage",
                            [&](Scope scope, const auto&) {
                                attempts.push_back(scope);
                                return false;
                            }),
                "stop at failed transform");
    ExpectEqual(std::size_t(1), attempts.size(), "settings not attempted after failure");
    ExpectTrue(attempts[0] == Scope::AuthoredTransforms, "authored transform is saved first");

    attempts.clear();
    ExpectFalse(state.Retry("stage",
                            [&](Scope scope, const auto&) {
                                attempts.push_back(scope);
                                if (scope == Scope::AuthoredTransforms) {
                                    state.ClearFailure(scope);
                                    return true;
                                }
                                return false;
                            }),
                "settings can fail after transform succeeds");
    ExpectEqual(std::size_t(2), attempts.size(), "both scopes attempted in order");
    ExpectTrue(attempts[1] == Scope::AllActorStates, "settings saved second");
    ExpectEqual(std::string("settings failed"), state.GetError(), "other failure remains visible");

    attempts.clear();
    ExpectTrue(state.Retry("stage",
                           [&](Scope scope, const auto&) {
                               attempts.push_back(scope);
                               state.ClearFailure(scope);
                               return true;
                           }),
               "remaining settings retry succeeds");
    ExpectEqual(std::size_t(1), attempts.size(), "successful transform not saved again");
    ExpectTrue(attempts[0] == Scope::AllActorStates, "only failed scope remains");
}

void StageChangeDoesNotRetryOrReuseOldNotification()
{
    StageActorSaveState state;
    state.RecordFailure(Scope::AllActorStates, "old stage", "old error",
                        [](const std::string&) { throw std::runtime_error("old stage notification must not run"); });
    bool attempted = false;
    ExpectFalse(state.Retry("new stage",
                            [&](Scope, const auto&) {
                                attempted = true;
                                return true;
                            }),
                "different stage rejected");
    ExpectFalse(attempted, "no cross-stage writes");
    ExpectFalse(static_cast<bool>(state.ResolveSavedCallback(Scope::AllActorStates, "new stage", {})),
                "no cross-stage history notification");
    state.Clear();
    ExpectTrue(state.GetError().empty(), "reload discards old error");
    ExpectTrue(state.Retry("new stage",
                           [&](Scope, const auto&) {
                               attempted = true;
                               return true;
                           }),
               "new stage has no stale retry");
    ExpectFalse(attempted, "discarded edit is not applied to replacement actors");
}

void NormalSaveCanReuseFailedCallbackWithoutMixingScopes()
{
    StageActorSaveState state;
    int notifications = 0;
    state.RecordFailure(Scope::AuthoredTransforms, "stage", "failed", [&](const std::string&) { ++notifications; });
    auto callback = state.ResolveSavedCallback(Scope::AuthoredTransforms, "stage", {});
    ExpectTrue(static_cast<bool>(callback), "normal retry inherits its history callback");
    callback("before");
    ExpectEqual(1, notifications, "inherited callback survives");
    ExpectFalse(static_cast<bool>(state.ResolveSavedCallback(Scope::AllActorStates, "stage", {})),
                "settings do not borrow transform callback");
    auto replacement = state.ResolveSavedCallback(Scope::AuthoredTransforms, "stage",
                                                  [&](const std::string&) { notifications += 10; });
    replacement("before");
    ExpectEqual(11, notifications, "explicit caller notification takes precedence");
}
void WholeStageRetrySupersedesPartialFailures()
{
    StageActorSaveState state;
    state.RecordFailure(Scope::AllActorStates, "stage", "settings", {});
    state.RecordFailure(Scope::AuthoredTransforms, "stage", "transforms", {});
    state.RecordFailure(Scope::StageSettings, "stage", "whole stage", {});
    ExpectEqual(std::string("whole stage"), state.GetError(), "whole-stage failure is actionable first");
    int attempts = 0;
    ExpectTrue(state.Retry("stage",
                           [&](Scope scope, const auto&) {
                               ++attempts;
                               ExpectTrue(scope == Scope::StageSettings, "one file replacement includes all edits");
                               state.Clear();
                               return true;
                           }),
               "whole-stage retry succeeds");
    ExpectEqual(1, attempts, "partial saves are not repeated after whole-stage commit");
    ExpectTrue(state.GetError().empty(), "all failures cleared together");
}

} // namespace

void RegisterStageActorSaveStateTests(TestRegistry& tests)
{
    tests.emplace_back("StageActorSave.WholeStageSupersedesPartialSaves", WholeStageRetrySupersedesPartialFailures);
    tests.emplace_back("StageActorSave.EmptyRetry", NoFailuresDoesNotInvokeSave);
    tests.emplace_back("StageActorSave.HistorySurvivesFailedRetry", RetryRetainsHistoryNotificationUntilSaveSucceeds);
    tests.emplace_back("StageActorSave.IndependentScopesAndRetryOrder", RetryOrdersScopesAndStopsAfterFailure);
    tests.emplace_back("StageActorSave.NoCrossStageRetry", StageChangeDoesNotRetryOrReuseOldNotification);
    tests.emplace_back("StageActorSave.NormalRetryReusesNotification",
                       NormalSaveCanReuseFailedCallbackWithoutMixingScopes);
}
