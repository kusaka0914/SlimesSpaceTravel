#include <GL/glew.h>

#include "system/UGCModeController.h"

#include "system/CameraSystem.h"
#include "system/UGCModeRuntime.h"
#include "system/UserDataPaths.h"

#include <glm/glm.hpp>

#include <filesystem>
#include <iostream>
#include <optional>

namespace {
constexpr int ugcStageNumber = 0;
constexpr int initialStageNumber = 0;
constexpr const char* initialStageYamlPath =
    "../assets/data/stage/house.yaml";
constexpr const char* completedTutorialProgressId =
    "tutorial:ugc_editor_completed";
constexpr const char* skippedTutorialProgressId =
    "tutorial:ugc_editor_skipped";
}

UGCModeController::UGCModeController(UGCModeRuntime& runtime)
    : mRuntime(runtime)
{
}

bool UGCModeController::StartMode()
{
    return StartModeWithStage(
        UserDataPaths::ResolveUGCWorkingStageFile().string(),
        false);
}

bool UGCModeController::StartModeFromTitleSelection()
{
    return HasSeenEditorTutorial()
        ? StartMode()
        : StartEditorTutorial();
}

bool UGCModeController::StartEditorTutorial()
{
    const std::filesystem::path tutorialTemplatePath =
        "../assets/data/stage/ugc_tutorial_template.yaml";
    const std::filesystem::path tutorialStagePath =
        UserDataPaths::ResolveUGCTutorialStageFile();
    std::error_code copyError;
    std::filesystem::copy_file(
        tutorialTemplatePath,
        tutorialStagePath,
        std::filesystem::copy_options::overwrite_existing,
        copyError);
    if (copyError) {
        std::cerr << "Failed to reset UGC editor tutorial: "
                  << copyError.message() << '\n';
        return false;
    }

    return StartModeWithStage(tutorialStagePath.string(), true);
}

bool UGCModeController::FinishEditorTutorial(bool wasCompleted)
{
    mRuntime.MarkProgressFlag(
        wasCompleted
            ? completedTutorialProgressId
            : skippedTutorialProgressId);
    mIsEditorTutorialActive = false;
    return StartMode();
}

void UGCModeController::OpenWorkBrowser()
{
    if (!mRuntime.IsTitleScene()) {
        return;
    }
    mSessionState.OpenWorkBrowser();
}

void UGCModeController::CloseWorkBrowser()
{
    mSessionState.CloseWorkBrowser();
}

void UGCModeController::StartPlaytest()
{
    if (!mSessionState.StartPlaytest()) {
        return;
    }
    mRuntime.SetDebugEditorShowing(false);
    mRuntime.SetFreeCameraMode(false);
    mRuntime.ReloadCurrentStage();
}

void UGCModeController::StartClearVerification(
    const std::string& workFileName)
{
    if (!mSessionState.StartVerification(workFileName)) {
        return;
    }
    StartPlaytest();
}

void UGCModeController::ReturnToEditor()
{
    if (!mSessionState.ReturnToEditor()) {
        return;
    }
    mRuntime.ReloadCurrentStage();
    mRuntime.SetDebugEditorShowing(true);
    mRuntime.SetFreeCameraMode(true);
    if (mIsEditorTutorialActive) {
        mRuntime.NotifyUGCTutorialReturnedFromPlaytest();
    }
}

void UGCModeController::ExitMode()
{
    mRuntime.RequestSceneFadeAction(
        [this]() { CompleteModeExit(false); });
}

bool UGCModeController::HandleGoalObtained()
{
    if (!mSessionState.IsModeActive()) {
        return false;
    }
    if (!mClearTransitionState.IsTransitionInProgress()) {
        // Actor走査中の再読込を避けるため、保存と遷移は次フレームで行う。
        mSessionState.MarkClearCompletionPending();
    }
    return true;
}

void UGCModeController::ProcessPendingClearCompletion()
{
    if (!mClearTransitionState.HasPendingCompletion()) {
        const std::optional<std::string> completedWorkFileName =
            mSessionState.ConsumeClearCompletion();
        if (completedWorkFileName) {
            mClearTransitionState.QueueCompletion(*completedWorkFileName);
        }
    }
    if (!mClearTransitionState.HasPendingCompletion()) {
        return;
    }

    const std::string completedWorkFileName =
        mClearTransitionState.GetCompletedWorkFileName();
    if (completedWorkFileName.empty()) {
        if (mRuntime.RequestSceneFadeAction(
                [this]() {
                    ReturnToEditor();
                    mClearTransitionState.CompleteTransition();
                })) {
            mClearTransitionState.BeginTransition();
        }
        return;
    }

    const auto completeVerificationAndReturnToBrowser =
        [this, completedWorkFileName]() {
            mRuntime.CompleteUGCVerification(completedWorkFileName);
            CompleteModeExit(true);
        };
    if (mRuntime.RequestSceneFadeAction(
            completeVerificationAndReturnToBrowser)) {
        mClearTransitionState.BeginTransition();
    }
}

void UGCModeController::ToggleDebugPanel()
{
    mSessionState.ToggleDebugPanel();
}

bool UGCModeController::IsModeActive() const
{
    return mSessionState.IsModeActive();
}

bool UGCModeController::IsPlaytestActive() const
{
    return mSessionState.IsPlaytestActive();
}

bool UGCModeController::IsVerificationActive() const
{
    return mSessionState.IsVerificationActive();
}

bool UGCModeController::IsDebugPanelShowing() const
{
    return mSessionState.IsDebugPanelShowing();
}

bool UGCModeController::IsWorkBrowserShowing() const
{
    return mSessionState.IsWorkBrowserShowing();
}

bool UGCModeController::IsOrthographicView() const
{
    return mSessionState.IsOrthographicView();
}

void UGCModeController::SetOrthographicView(bool isOrthographicView)
{
    mSessionState.SetOrthographicView(isOrthographicView);
}

bool UGCModeController::IsEditorTutorialActive() const
{
    return mIsEditorTutorialActive;
}

bool UGCModeController::StartModeWithStage(
    const std::string& yamlPath,
    bool isTutorial)
{
    if (!mRuntime.LoadDebugStage(ugcStageNumber, yamlPath)) {
        return false;
    }

    mIsEditorTutorialActive = isTutorial;
    mRuntime.ClosePauseMenu();
    mSessionState.EnterEditor();
    mRuntime.SetUGCOrthographicHalfHeight(20.0f);
    mRuntime.SetDebugEditorShowing(true);
    mRuntime.SetFreeCameraMode(true);
    mRuntime.StartPlayingScene();

    CameraPose pose;
    pose.position = glm::vec3(0.0f, 30.0f, 0.0f);
    pose.target = glm::vec3(0.0f);
    pose.up = glm::vec3(0.0f, 0.0f, -1.0f);
    pose.fieldOfViewDegrees = 55.0f;
    mRuntime.SetDebugCameraPose(pose);
    return true;
}

bool UGCModeController::HasSeenEditorTutorial() const
{
    return mRuntime.HasProgressFlag(
               completedTutorialProgressId) ||
        mRuntime.HasProgressFlag(
               skippedTutorialProgressId);
}

void UGCModeController::CompleteModeExit(bool shouldOpenWorkBrowser)
{
    mClearTransitionState.CompleteTransition();
    mSessionState.Exit();
    mIsEditorTutorialActive = false;
    mRuntime.SetDebugEditorShowing(false);
    mRuntime.SetFreeCameraMode(false);

    if (mRuntime.LoadDebugStage(initialStageNumber, initialStageYamlPath)) {
        mRuntime.EnterTitleAtFadeMidpoint();
        mRuntime.TryChangeBGM();
        if (shouldOpenWorkBrowser) {
            OpenWorkBrowser();
        }
    }
}
