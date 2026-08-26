#include "TestSupport.h"

#include "gfx/debug/ugc/UGCSessionState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void EnterEditorActivatesEditingViewAndClosesBrowser()
{
    UGCSessionState sessionState;
    sessionState.OpenWorkBrowser();

    sessionState.EnterEditor();

    ExpectTrue(sessionState.IsModeActive(), "mode active");
    ExpectTrue(sessionState.IsOrthographicView(), "orthographic view");
    ExpectFalse(sessionState.IsDebugPanelShowing(), "debug panel");
    ExpectFalse(sessionState.IsWorkBrowserShowing(), "work browser");
}

void PlaytestIsRejectedOutsideUGCMode()
{
    UGCSessionState sessionState;

    ExpectFalse(sessionState.StartPlaytest(), "playtest start result");
    ExpectFalse(sessionState.IsModeActive(), "mode active");
}

void PlaytestHidesEditingViews()
{
    UGCSessionState sessionState;
    sessionState.EnterEditor();
    sessionState.ToggleDebugPanel();

    const bool wasStarted = sessionState.StartPlaytest();

    ExpectTrue(wasStarted, "playtest start result");
    ExpectFalse(sessionState.IsOrthographicView(), "orthographic view");
    ExpectFalse(sessionState.IsDebugPanelShowing(), "debug panel");
}

void VerificationRequiresActiveModeAndFileName()
{
    UGCSessionState sessionState;

    ExpectFalse(
        sessionState.StartVerification("work.yaml"),
        "inactive verification");
    sessionState.EnterEditor();
    ExpectFalse(
        sessionState.StartVerification(""),
        "empty file verification");
    ExpectTrue(
        sessionState.StartVerification("work.yaml"),
        "valid verification");
}

void ClearCompletionReturnsVerificationFileOnce()
{
    UGCSessionState sessionState;
    sessionState.EnterEditor();
    sessionState.StartVerification("work.yaml");
    sessionState.MarkClearCompletionPending();

    const std::optional<std::string> workFileName =
        sessionState.ConsumeClearCompletion();

    ExpectTrue(workFileName.has_value(), "completion exists");
    ExpectEqual(std::string("work.yaml"), *workFileName, "work file name");
    ExpectFalse(
        sessionState.ConsumeClearCompletion().has_value(),
        "completion consumed once");
}

void NormalPlaytestClearCompletionHasEmptyVerificationFile()
{
    UGCSessionState sessionState;
    sessionState.EnterEditor();
    sessionState.MarkClearCompletionPending();

    const std::optional<std::string> workFileName =
        sessionState.ConsumeClearCompletion();

    ExpectTrue(workFileName.has_value(), "completion exists");
    ExpectTrue(workFileName->empty(), "verification file is empty");
}

void ReturnToEditorCancelsPendingVerification()
{
    UGCSessionState sessionState;
    sessionState.EnterEditor();
    sessionState.StartVerification("work.yaml");
    sessionState.MarkClearCompletionPending();

    const bool wasReturned = sessionState.ReturnToEditor();

    ExpectTrue(wasReturned, "return result");
    ExpectTrue(sessionState.IsModeActive(), "mode remains active");
    ExpectTrue(sessionState.IsOrthographicView(), "orthographic view");
    ExpectFalse(
        sessionState.ConsumeClearCompletion().has_value(),
        "pending completion cancelled");
}

void ExitClearsAllVisibleUGCState()
{
    UGCSessionState sessionState;
    sessionState.EnterEditor();
    sessionState.ToggleDebugPanel();
    sessionState.OpenWorkBrowser();

    sessionState.Exit();

    ExpectFalse(sessionState.IsModeActive(), "mode active");
    ExpectFalse(sessionState.IsDebugPanelShowing(), "debug panel");
    ExpectFalse(sessionState.IsWorkBrowserShowing(), "work browser");
    ExpectFalse(sessionState.IsOrthographicView(), "orthographic view");
}

}

void RegisterUGCSessionStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back("UGCSessionState.EnterEditorActivatesEditingViewAndClosesBrowser", EnterEditorActivatesEditingViewAndClosesBrowser);
    tests.emplace_back("UGCSessionState.PlaytestIsRejectedOutsideUGCMode", PlaytestIsRejectedOutsideUGCMode);
    tests.emplace_back("UGCSessionState.PlaytestHidesEditingViews", PlaytestHidesEditingViews);
    tests.emplace_back("UGCSessionState.VerificationRequiresActiveModeAndFileName", VerificationRequiresActiveModeAndFileName);
    tests.emplace_back("UGCSessionState.ClearCompletionReturnsVerificationFileOnce", ClearCompletionReturnsVerificationFileOnce);
    tests.emplace_back("UGCSessionState.NormalPlaytestClearCompletionHasEmptyVerificationFile", NormalPlaytestClearCompletionHasEmptyVerificationFile);
    tests.emplace_back("UGCSessionState.ReturnToEditorCancelsPendingVerification", ReturnToEditorCancelsPendingVerification);
    tests.emplace_back("UGCSessionState.ExitClearsAllVisibleUGCState", ExitClearsAllVisibleUGCState);
}
