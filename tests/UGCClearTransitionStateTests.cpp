#include "TestSupport.h"

#include "gfx/debug/ugc/UGCClearTransitionState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void NormalPlaytestQueuesAnEmptyWorkFileName()
{
    UGCClearTransitionState state;

    state.QueueCompletion("");

    ExpectTrue(state.HasPendingCompletion(), "normal completion is pending");
    ExpectEqual("", state.GetCompletedWorkFileName(), "normal work file name");
}

void VerificationCompletionPreservesWorkFileName()
{
    UGCClearTransitionState state;

    state.QueueCompletion("challenge.yaml");

    ExpectEqual(
        "challenge.yaml",
        state.GetCompletedWorkFileName(),
        "verification work file name");
}

void BeginningTransitionConsumesOnlyTheQueuedCompletion()
{
    UGCClearTransitionState state;
    state.QueueCompletion("challenge.yaml");

    state.BeginTransition();

    ExpectTrue(state.IsTransitionInProgress(), "transition state");
    ExpectFalse(state.HasPendingCompletion(), "completion consumed");
    state.QueueCompletion("second.yaml");
    ExpectFalse(state.HasPendingCompletion(), "completion ignored during transition");
}

void CompletingTransitionAllowsNextCompletion()
{
    UGCClearTransitionState state;
    state.QueueCompletion("first.yaml");
    state.BeginTransition();
    state.CompleteTransition();
    state.QueueCompletion("second.yaml");

    ExpectFalse(state.IsTransitionInProgress(), "completed transition state");
    ExpectTrue(state.HasPendingCompletion(), "next completion pending");
    ExpectEqual("second.yaml", state.GetCompletedWorkFileName(), "next work file");
}

}

void RegisterUGCClearTransitionStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCClearTransitionState.NormalPlaytestQueuesAnEmptyWorkFileName",
        NormalPlaytestQueuesAnEmptyWorkFileName);
    tests.emplace_back(
        "UGCClearTransitionState.VerificationCompletionPreservesWorkFileName",
        VerificationCompletionPreservesWorkFileName);
    tests.emplace_back(
        "UGCClearTransitionState.BeginningTransitionConsumesOnlyTheQueuedCompletion",
        BeginningTransitionConsumesOnlyTheQueuedCompletion);
    tests.emplace_back(
        "UGCClearTransitionState.CompletingTransitionAllowsNextCompletion",
        CompletingTransitionAllowsNextCompletion);
}
