#include "TestSupport.h"

#include "system/UGCModeController.h"
#include "system/UGCModeRuntime.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

class FakeUGCModeRuntime : public UGCModeRuntime {
public:
    bool IsTitleScene() const override { return isTitleScene; }

    void SetDebugEditorShowing(bool isShowing) override
    {
        isDebugEditorShowing = isShowing;
    }

    void SetFreeCameraMode(bool isEnabled) override
    {
        isFreeCameraMode = isEnabled;
    }

    void ReloadCurrentStage() override { ++reloadCount; }

    bool LoadDebugStage(
        int stageNumber,
        const std::string& yamlPath) override
    {
        loadedStageNumber = stageNumber;
        loadedStageYamlPath = yamlPath;
        return shouldLoadStage;
    }

    void ClosePauseMenu() override { didClosePauseMenu = true; }

    void SetUGCOrthographicHalfHeight(float halfHeight) override
    {
        orthographicHalfHeight = halfHeight;
    }

    void StartPlayingScene() override
    {
        didStartPlayingScene = true;
        ++startPlayingSceneCount;
    }

    void StartUGCStageClearPresentation() override
    {
        didStartUGCStageClearPresentation = true;
    }

    void SetDebugCameraPose(const CameraPose&) override
    {
        didSetDebugCameraPose = true;
    }

    bool RequestSceneFadeAction(
        const std::function<void()>& fadeAction) override
    {
        pendingFadeAction = fadeAction;
        return shouldAcceptFadeAction;
    }

    void NotifyUGCTutorialReturnedFromPlaytest() override
    {
        didNotifyTutorialReturn = true;
    }

    void CompleteUGCVerification(
        const std::string& workFileName) override
    {
        completedWorkFileName = workFileName;
    }

    bool HasProgressFlag(const std::string& progressId) const override
    {
        for (const std::string& savedProgressId : progressIds) {
            if (savedProgressId == progressId) {
                return true;
            }
        }
        return false;
    }

    void MarkProgressFlag(const std::string& progressId) override
    {
        progressIds.push_back(progressId);
    }

    void EnterTitleAtFadeMidpoint() override
    {
        isTitleScene = true;
        didEnterTitle = true;
    }

    void TryChangeBGM() override { didTryChangeBGM = true; }

    void CompletePendingFade()
    {
        const std::function<void()> fadeAction = pendingFadeAction;
        pendingFadeAction = {};
        if (fadeAction) {
            fadeAction();
        }
    }

    bool isTitleScene = true;
    bool shouldLoadStage = true;
    bool shouldAcceptFadeAction = true;
    bool isDebugEditorShowing = false;
    bool isFreeCameraMode = false;
    bool didClosePauseMenu = false;
    bool didStartPlayingScene = false;
    bool didStartUGCStageClearPresentation = false;
    bool didSetDebugCameraPose = false;
    bool didNotifyTutorialReturn = false;
    bool didEnterTitle = false;
    bool didTryChangeBGM = false;
    int reloadCount = 0;
    int startPlayingSceneCount = 0;
    int loadedStageNumber = -1;
    float orthographicHalfHeight = 0.0f;
    std::string loadedStageYamlPath;
    std::string completedWorkFileName;
    std::vector<std::string> progressIds;
    std::function<void()> pendingFadeAction;
};

void StartingModeConfiguresEditorRuntime()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);

    const bool didStart = controller.StartMode();

    ExpectTrue(didStart, "mode start result");
    ExpectTrue(controller.IsModeActive(), "UGC mode active");
    ExpectEqual(0, runtime.loadedStageNumber, "UGC stage number");
    ExpectTrue(
        !runtime.loadedStageYamlPath.empty(),
        "UGC working stage path");
    ExpectTrue(runtime.didClosePauseMenu, "pause menu closed");
    ExpectTrue(runtime.isDebugEditorShowing, "debug editor showing");
    ExpectTrue(runtime.isFreeCameraMode, "free camera enabled");
    ExpectTrue(runtime.didStartPlayingScene, "playing scene started");
    ExpectTrue(runtime.didSetDebugCameraPose, "debug camera pose set");
    ExpectNear(
        20.0f,
        runtime.orthographicHalfHeight,
        0.0001f,
        "orthographic half height");
}

void ReturningFromPlaytestRestoresEditorRuntime()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();

    controller.StartPlaytest();
    ExpectTrue(controller.IsPlaytestActive(), "playtest active");
    ExpectFalse(runtime.isDebugEditorShowing, "editor hidden for playtest");
    ExpectFalse(runtime.isFreeCameraMode, "free camera disabled for playtest");

    controller.ReturnToEditor();
    ExpectFalse(controller.IsPlaytestActive(), "playtest ended");
    ExpectTrue(runtime.isDebugEditorShowing, "editor restored");
    ExpectTrue(runtime.isFreeCameraMode, "free camera restored");
    ExpectEqual(2, runtime.reloadCount, "stage reload count");
    ExpectEqual(
        3,
        runtime.startPlayingSceneCount,
        "playing scene restored after editor return");
}

void VerificationCompletionWaitsForFadeAndReturnsToBrowser()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartClearVerification("verified_work.yaml");

    ExpectTrue(controller.HandleGoalObtained(), "goal handled by UGC mode");
    controller.ProcessPendingClearCompletion();

    ExpectTrue(
        runtime.didStartUGCStageClearPresentation,
        "stage clear presentation started");
    ExpectFalse(
        static_cast<bool>(runtime.pendingFadeAction),
        "fade waits for collection animation");

    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();

    ExpectTrue(
        static_cast<bool>(runtime.pendingFadeAction),
        "verification fade queued");
    ExpectTrue(
        runtime.completedWorkFileName.empty(),
        "verification not completed before fade");

    runtime.CompletePendingFade();

    ExpectEqual(
        std::string("verified_work.yaml"),
        runtime.completedWorkFileName,
        "completed work file");
    ExpectTrue(runtime.didEnterTitle, "returned to title");
    ExpectTrue(runtime.didTryChangeBGM, "title BGM requested");
    ExpectTrue(controller.IsWorkBrowserShowing(), "work browser opened");
    ExpectFalse(controller.IsModeActive(), "UGC mode exited");
}

void EditorPreviewClearReturnsToEditorAfterCollectionAndFade()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartPlaytest();

    controller.HandleGoalObtained();
    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();

    ExpectTrue(
        static_cast<bool>(runtime.pendingFadeAction),
        "editor preview fade queued");
    ExpectFalse(runtime.isDebugEditorShowing, "editor hidden before fade");
    runtime.CompletePendingFade();
    ExpectTrue(runtime.isDebugEditorShowing, "editor restored after fade");
    ExpectFalse(controller.IsPlaytestActive(), "preview ended");
    ExpectEqual(
        3,
        runtime.startPlayingSceneCount,
        "stage clear state ended before editor rendering");
}

void SavedWorkClearShowsResultMenuAfterCollectionAndFade()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartSavedWorkPlaytest();

    controller.HandleGoalObtained();
    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();
    runtime.CompletePendingFade();

    ExpectTrue(controller.IsClearResultShowing(), "result menu showing");
    ExpectTrue(controller.IsPlaytestActive(), "saved work remains in playtest");
    controller.MoveClearResultSelection(1);
    ExpectEqual(1, controller.GetClearResultSelection(), "result selection");
}

void SavedWorkResultRetryRestartsPlaytest()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartSavedWorkPlaytest();
    controller.HandleGoalObtained();
    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();
    runtime.CompletePendingFade();

    const int reloadCountBeforeRetry = runtime.reloadCount;
    controller.ExecuteClearResultSelection();
    runtime.CompletePendingFade();

    ExpectFalse(controller.IsClearResultShowing(), "retry closes result");
    ExpectTrue(controller.IsPlaytestActive(), "retry starts playtest");
    ExpectEqual(
        reloadCountBeforeRetry + 1,
        runtime.reloadCount,
        "retry reloads saved stage");
}

void SavedWorkResultEditReturnsToEditor()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartSavedWorkPlaytest();
    controller.HandleGoalObtained();
    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();
    runtime.CompletePendingFade();

    controller.MoveClearResultSelection(1);
    controller.ExecuteClearResultSelection();
    runtime.CompletePendingFade();

    ExpectFalse(controller.IsClearResultShowing(), "edit closes result");
    ExpectFalse(controller.IsPlaytestActive(), "edit ends playtest");
    ExpectTrue(runtime.isDebugEditorShowing, "edit restores editor");
}

void SavedWorkResultTitleExitsUGCMode()
{
    FakeUGCModeRuntime runtime;
    UGCModeController controller(runtime);
    controller.StartMode();
    controller.StartSavedWorkPlaytest();
    controller.HandleGoalObtained();
    controller.HandleGoalCollectionFinished();
    controller.ProcessPendingClearCompletion();
    runtime.CompletePendingFade();

    controller.MoveClearResultSelection(2);
    controller.ExecuteClearResultSelection();
    runtime.CompletePendingFade();

    ExpectFalse(controller.IsModeActive(), "title exits UGC mode");
    ExpectTrue(runtime.didEnterTitle, "title scene entered");
}

}

void RegisterUGCModeControllerTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "UGCModeController.StartingModeConfiguresEditorRuntime",
        StartingModeConfiguresEditorRuntime);
    tests.emplace_back(
        "UGCModeController.ReturningFromPlaytestRestoresEditorRuntime",
        ReturningFromPlaytestRestoresEditorRuntime);
    tests.emplace_back(
        "UGCModeController.VerificationCompletionWaitsForFadeAndReturnsToBrowser",
        VerificationCompletionWaitsForFadeAndReturnsToBrowser);
    tests.emplace_back(
        "UGCModeController.EditorPreviewClearReturnsToEditorAfterCollectionAndFade",
        EditorPreviewClearReturnsToEditorAfterCollectionAndFade);
    tests.emplace_back(
        "UGCModeController.SavedWorkClearShowsResultMenuAfterCollectionAndFade",
        SavedWorkClearShowsResultMenuAfterCollectionAndFade);
    tests.emplace_back(
        "UGCModeController.SavedWorkResultRetryRestartsPlaytest",
        SavedWorkResultRetryRestartsPlaytest);
    tests.emplace_back(
        "UGCModeController.SavedWorkResultEditReturnsToEditor",
        SavedWorkResultEditReturnsToEditor);
    tests.emplace_back(
        "UGCModeController.SavedWorkResultTitleExitsUGCMode",
        SavedWorkResultTitleExitsUGCMode);
}
