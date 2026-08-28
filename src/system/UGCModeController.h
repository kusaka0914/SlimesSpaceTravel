#pragma once

#include "gfx/debug/ugc/UGCClearTransitionState.h"
#include "gfx/debug/ugc/UGCSessionState.h"

#include <string>

class UGCModeRuntime;

class UGCModeController {
public:
    explicit UGCModeController(UGCModeRuntime& runtime);

    bool StartMode();
    bool StartModeFromTitleSelection();
    bool StartEditorTutorial();
    bool FinishEditorTutorial(bool wasCompleted);
    void OpenWorkBrowser();
    void CloseWorkBrowser();
    void StartPlaytest();
    void StartClearVerification(const std::string& workFileName);
    void ReturnToEditor();
    void ExitMode();
    bool HandleGoalObtained();
    void ProcessPendingClearCompletion();
    void ToggleDebugPanel();

    bool IsModeActive() const;
    bool IsPlaytestActive() const;
    bool IsVerificationActive() const;
    bool IsDebugPanelShowing() const;
    bool IsWorkBrowserShowing() const;
    bool IsOrthographicView() const;
    void SetOrthographicView(bool isOrthographicView);
    bool IsEditorTutorialActive() const;

private:
    bool StartModeWithStage(
        const std::string& yamlPath,
        bool isTutorial);
    bool HasSeenEditorTutorial() const;
    void CompleteModeExit(bool shouldOpenWorkBrowser);

    UGCModeRuntime& mRuntime;
    UGCSessionState mSessionState;
    UGCClearTransitionState mClearTransitionState;
    bool mIsEditorTutorialActive = false;
};
