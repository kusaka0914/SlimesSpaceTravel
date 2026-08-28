#include "gfx/debug/ugc/UGCWorkController.h"

#include "Game.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCWorkMetadata.h"
#include "gfx/debug/ugc/UGCWorkState.h"

#include <algorithm>
#include <utility>
#include <yaml-cpp/yaml.h>

UGCWorkController::UGCWorkController(
    DebugEditorContext& context,
    UGCWorkState& state,
    std::function<void()> reloadSelectedWork)
    : mContext(context),
      mState(state),
      mFileService(context),
      mReloadSelectedWork(std::move(reloadSelectedWork))
{
}

void UGCWorkController::RefreshWorkList()
{
    const std::optional<std::vector<std::string>> foundFileNames =
        mFileService.FindSavedWorkFileNames();
    if (!foundFileNames) {
        mState.workFileNames.clear();
        mState.selectedWorkIndex = -1;
        return;
    }

    mState.workFileNames = *foundFileNames;
    if (mState.workFileNames.empty()) {
        mState.selectedWorkIndex = -1;
    } else {
        mState.selectedWorkIndex = std::clamp(
            mState.selectedWorkIndex,
            0,
            static_cast<int>(mState.workFileNames.size()) - 1);
    }
    mState.hasLoadedWorkList = true;
}

void UGCWorkController::SetWorkNameInput(const std::string& workName)
{
    mState.workName.fill('\0');
    const std::size_t copiedCharacterCount = std::min(
        workName.size(),
        mState.workName.size() - 1);
    std::copy_n(
        workName.begin(),
        copiedCharacterCount,
        mState.workName.begin());
}

void UGCWorkController::SetCurrentWork(
    const std::string& fileName,
    const std::string& displayName)
{
    mState.currentWorkFileName = fileName;
    mState.currentWorkDisplayName = displayName;
    SetWorkNameInput(displayName);
    mState.isNamingNewSave = false;
}

void UGCWorkController::SynchronizeCurrentWorkIdentity()
{
    if (mState.hasSynchronizedCurrentWork) {
        return;
    }
    mState.hasSynchronizedCurrentWork = true;

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return;
    }

    const std::optional<std::string> savedDisplayName =
        UGCWorkMetadata::FindDisplayName(stageYaml);
    std::optional<std::string> savedFileName =
        UGCWorkMetadata::FindFileName(stageYaml);
    if (!savedFileName && savedDisplayName) {
        savedFileName =
            mFileService.CreateWorkFileName(*savedDisplayName);
    }
    if (!savedFileName ||
        std::find(
            mState.workFileNames.begin(),
            mState.workFileNames.end(),
            *savedFileName) == mState.workFileNames.end()) {
        if (savedDisplayName) {
            SetWorkNameInput(*savedDisplayName);
        }
        return;
    }

    SetCurrentWork(
        *savedFileName,
        savedDisplayName.value_or(
            mFileService.ResolveDisplayName(*savedFileName)));
}

bool UGCWorkController::SaveWorkToFile(
    const std::string& displayName,
    const std::string& fileName)
{
    if (!mFileService.SaveCurrentWork(
            displayName,
            fileName,
            mState.saveErrorMessage)) {
        return false;
    }

    SetCurrentWork(fileName, displayName);
    mState.shouldRefreshWorkList = true;
    return true;
}

bool UGCWorkController::SaveAsNamedWork()
{
    const std::string displayName = mState.workName.data();
    return SaveWorkToFile(
        displayName,
        mFileService.CreateWorkFileName(displayName));
}

bool UGCWorkController::OverwriteCurrentWork()
{
    return mState.currentWorkFileName &&
        SaveWorkToFile(
            mState.currentWorkDisplayName,
            *mState.currentWorkFileName);
}

bool UGCWorkController::SaveCurrentWorkForVerification(
    std::string& outWorkFileName,
    std::string& outErrorMessage)
{
    if (!mState.currentWorkFileName) {
        outWorkFileName.clear();
        outErrorMessage = "先に作品名を付けて保存してください";
        return false;
    }
    if (!OverwriteCurrentWork()) {
        outWorkFileName.clear();
        outErrorMessage = mState.saveErrorMessage;
        return false;
    }

    outWorkFileName = *mState.currentWorkFileName;
    outErrorMessage.clear();
    return true;
}

void UGCWorkController::StartVerification(std::string& outStatusMessage)
{
    if (!mState.hasLoadedWorkList) {
        RefreshWorkList();
    }
    SynchronizeCurrentWorkIdentity();

    YAML::Node stageYaml;
    const bool wasLoaded =
        StageYamlRepository::LoadCurrentStage(mContext, stageYaml);
    if (!wasLoaded || !UGCWorkMetadata::HasGoal(stageYaml)) {
        outStatusMessage = "完成チェックにはゴールを置いてください";
        return;
    }

    std::string workFileName;
    std::string saveErrorMessage;
    if (!SaveCurrentWorkForVerification(
            workFileName, saveErrorMessage)) {
        outStatusMessage =
            "下書きを保存できませんでした: " + saveErrorMessage;
        return;
    }
    mContext.game->StartUGCClearVerification(workFileName);
}

bool UGCWorkController::CompleteVerification(
    const std::string& workFileName)
{
    const bool wasCompleted =
        mFileService.CompleteVerification(workFileName);
    RefreshWorkList();
    return wasCompleted;
}

bool UGCWorkController::HasUnsavedChanges() const
{
    return mFileService.HasUnsavedChanges();
}

const std::string* UGCWorkController::FindSelectedWorkFileName() const
{
    if (mState.selectedWorkIndex < 0 ||
        mState.selectedWorkIndex >= static_cast<int>(mState.workFileNames.size())) {
        return nullptr;
    }
    return &mState.workFileNames[mState.selectedWorkIndex];
}

bool UGCWorkController::CopySelectedWorkToWorkingFile()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    if (!selectedFileName) {
        return false;
    }
    const std::string copiedFileName = *selectedFileName;
    if (!mFileService.CopyToWorkingFile(copiedFileName)) {
        return false;
    }

    SetCurrentWork(
        copiedFileName,
        mFileService.ResolveDisplayName(copiedFileName));
    return true;
}

bool UGCWorkController::LoadSelectedWork()
{
    if (!CopySelectedWorkToWorkingFile()) {
        return false;
    }
    mReloadSelectedWork();
    return true;
}

bool UGCWorkController::CreateNewWorkingStage()
{
    if (!mFileService.ResetWorkingStage(mState.saveErrorMessage)) {
        return false;
    }

    mState.currentWorkFileName.reset();
    mState.currentWorkDisplayName.clear();
    SetWorkNameInput("新しいステージ");
    mState.isNamingNewSave = true;
    mState.hasSynchronizedCurrentWork = true;
    mReloadSelectedWork();
    return true;
}

bool UGCWorkController::DuplicateSelectedWork()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    if (!selectedFileName ||
        !mFileService.DuplicateWork(*selectedFileName)) {
        return false;
    }
    RefreshWorkList();
    return true;
}

bool UGCWorkController::DeleteSelectedWork()
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    if (!selectedFileName) {
        return false;
    }
    const std::string deletedFileName = *selectedFileName;
    const bool wasDeleted = mFileService.DeleteWork(deletedFileName);
    if (wasDeleted &&
        mState.currentWorkFileName == deletedFileName) {
        mState.currentWorkFileName.reset();
        mState.currentWorkDisplayName.clear();
        mState.isNamingNewSave = true;
    }
    RefreshWorkList();
    return wasDeleted;
}

bool UGCWorkController::IsSelectedWorkVerified() const
{
    const std::string* selectedFileName = FindSelectedWorkFileName();
    return selectedFileName &&
        mFileService.IsClearVerified(*selectedFileName);
}


bool UGCWorkController::IsWorkVerified(const std::string& fileName) const
{
    return mFileService.IsClearVerified(fileName);
}

std::string UGCWorkController::ResolveDisplayName(
    const std::string& fileName) const
{
    return mFileService.ResolveDisplayName(fileName);
}

