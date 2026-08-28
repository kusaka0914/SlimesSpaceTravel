#pragma once

#include "gfx/debug/ugc/UGCWorkFileService.h"

#include <array>
#include <functional>
#include <optional>
#include <string>
#include <vector>

struct DebugEditorContext;

class UGCWorkPanel {
public:
    UGCWorkPanel(
        DebugEditorContext& context,
        std::function<void()> reloadSelectedWork);

    void DrawManagement(std::string& outStatusMessage);
    void DrawBrowser();
    void StartVerification(std::string& outStatusMessage);
    bool CompleteVerification(const std::string& workFileName);
    bool HasUnsavedChanges() const;

private:
    void RefreshWorkList();
    void SynchronizeCurrentWorkIdentity();
    void SetWorkNameInput(const std::string& workName);
    void SetCurrentWork(
        const std::string& fileName,
        const std::string& displayName);
    bool SaveWorkToFile(
        const std::string& displayName,
        const std::string& fileName);
    bool SaveAsNamedWork();
    bool OverwriteCurrentWork();
    bool SaveCurrentWorkForVerification(
        std::string& outWorkFileName,
        std::string& outErrorMessage);
    bool CreateNewWorkingStage();
    bool LoadSelectedWork();
    bool CopySelectedWorkToWorkingFile();
    bool DuplicateSelectedWork();
    bool DeleteSelectedWork();
    bool IsSelectedWorkVerified() const;
    const std::string* FindSelectedWorkFileName() const;
    void DrawCurrentWorkSaveControls(std::string& outStatusMessage);
    void DrawSavedWorkList(float listHeight);
    void DrawManagementDeleteConfirmation(std::string& outStatusMessage);
    bool DrawNewWorkConfirmation(std::string& outStatusMessage);

    DebugEditorContext& mContext;
    UGCWorkFileService mFileService;
    std::function<void()> mReloadSelectedWork;
    std::array<char, 96> mWorkName{"新しいステージ"};
    std::vector<std::string> mWorkFileNames;
    std::optional<std::string> mCurrentWorkFileName;
    std::string mCurrentWorkDisplayName;
    std::string mSaveErrorMessage;
    int mSelectedWorkIndex = -1;
    bool mHasLoadedWorkList = false;
    bool mShouldRefreshWorkList = false;
    bool mHasSynchronizedCurrentWork = false;
    bool mIsNamingNewSave = true;
};
