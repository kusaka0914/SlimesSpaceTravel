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
    StartPlaytest(UGCPlaytestPurpose::EditorPreview);
}

void UGCModeController::StartSavedWorkPlaytest()
{
    StartPlaytest(UGCPlaytestPurpose::SavedWork);
}

void UGCModeController::StartPlaytest(UGCPlaytestPurpose purpose)
{
    if (!mSessionState.StartPlaytest(purpose)) {
        return;
    }
    mRuntime.SetDebugEditorShowing(false);
    mRuntime.SetFreeCameraMode(false);
    mRuntime.ReloadCurrentStage();
    mRuntime.StartPlayingScene();
}

void UGCModeController::StartClearVerification(
    const std::string& workFileName)
{
    if (!mSessionState.StartVerification(workFileName)) {
        return;
    }
    StartPlaytest(UGCPlaytestPurpose::ClearVerification);
}

void UGCModeController::ReturnToEditor()
{
    if (!mSessionState.ReturnToEditor()) {
        return;
    }
    mRuntime.ReloadCurrentStage();
    mRuntime.StartPlayingScene();
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
    mRuntime.StartUGCStageClearPresentation();
    return true;
}

void UGCModeController::HandleGoalCollectionFinished()
{
    if (!mSessionState.IsModeActive() ||
        mClearTransitionState.IsTransitionInProgress()) {
        return;
    }

    // Star更新中にステージを再読込しないよう、実際の遷移は次フレームで行う。
    mSessionState.MarkClearCompletionPending();
}

void UGCModeController::ProcessPendingClearCompletion()
{
    if (!mClearTransitionState.HasPendingCompletion()) {
        const std::optional<std::string> completedWorkFileName =
            mSessionState.ConsumeClearCompletion();
        if (completedWorkFileName) {
            UGCClearDestination destination = UGCClearDestination::Editor;
            switch (mSessionState.GetPlaytestPurpose()) {
            case UGCPlaytestPurpose::ClearVerification:
                destination = UGCClearDestination::WorkBrowser;
                break;
            case UGCPlaytestPurpose::SavedWork:
                destination = UGCClearDestination::ResultMenu;
                break;
            case UGCPlaytestPurpose::EditorPreview:
                break;
            }
            mClearTransitionState.QueueCompletion(
                *completedWorkFileName,
                destination);
        }
    }
    if (!mClearTransitionState.HasPendingCompletion()) {
        return;
    }

    const std::string completedWorkFileName =
        mClearTransitionState.GetCompletedWorkFileName();
    const UGCClearDestination destination =
        mClearTransitionState.GetDestination();
    const auto completeClearTransition =
        [this, completedWorkFileName, destination]() {
            switch (destination) {
            case UGCClearDestination::Editor:
                ReturnToEditor();
                break;
            case UGCClearDestination::WorkBrowser:
                mRuntime.CompleteUGCVerification(completedWorkFileName);
                CompleteModeExit(true);
                break;
            case UGCClearDestination::ResultMenu:
                mSessionState.ShowClearResult();
                break;
            }
            mClearTransitionState.CompleteTransition();
        };
    if (mRuntime.RequestSceneFadeAction(
            completeClearTransition)) {
        mClearTransitionState.BeginTransition();
    }
}

void UGCModeController::MoveClearResultSelection(int delta)
{
    constexpr int clearResultItemCount = 3;
    mSessionState.MoveClearResultSelection(delta, clearResultItemCount);
}

void UGCModeController::ExecuteClearResultSelection()
{
    if (!mSessionState.IsClearResultShowing()) {
        return;
    }

    switch (mSessionState.GetClearResultSelection()) {
    case 0:
        mRuntime.RequestSceneFadeAction(
            [this]() { StartSavedWorkPlaytest(); });
        break;
    case 1:
        mRuntime.RequestSceneFadeAction([this]() { ReturnToEditor(); });
        break;
    case 2:
        ExitMode();
        break;
    default:
        break;
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

bool UGCModeController::IsClearResultShowing() const
{
    return mSessionState.IsClearResultShowing();
}

int UGCModeController::GetClearResultSelection() const
{
    return mSessionState.GetClearResultSelection();
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
