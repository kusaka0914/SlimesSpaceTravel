#pragma once

#include "gfx/debug/ugc/UGCWorkFileService.h"

#include <functional>
#include <string>

struct DebugEditorContext;
struct UGCWorkState;

class UGCWorkController {
public:
    UGCWorkController(
        DebugEditorContext& context,
        UGCWorkState& state,
        std::function<void()> reloadSelectedWork);

    void RefreshWorkList();
    void SynchronizeCurrentWorkIdentity();
    void SetWorkNameInput(const std::string& workName);
    bool SaveAsNamedWork();
    bool OverwriteCurrentWork();
    void StartVerification(std::string& outStatusMessage);
    bool CompleteVerification(const std::string& workFileName);
    bool HasUnsavedChanges() const;
    const std::string* FindSelectedWorkFileName() const;
    bool CopySelectedWorkToWorkingFile();
    bool LoadSelectedWork();
    bool CreateNewWorkingStage();
    bool DuplicateSelectedWork();
    bool DeleteSelectedWork();
    bool IsSelectedWorkVerified() const;
    bool IsWorkVerified(const std::string& fileName) const;
    std::string ResolveDisplayName(const std::string& fileName) const;

private:
    void SetCurrentWork(
        const std::string& fileName,
        const std::string& displayName);
    bool SaveWorkToFile(
        const std::string& displayName,
        const std::string& fileName);
    bool SaveCurrentWorkForVerification(
        std::string& outWorkFileName,
        std::string& outErrorMessage);

    DebugEditorContext& mContext;
    UGCWorkState& mState;
    UGCWorkFileService mFileService;
    std::function<void()> mReloadSelectedWork;
};
