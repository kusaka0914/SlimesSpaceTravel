#pragma once

#include "gfx/debug/ugc/UGCWorkFileService.h"

#include <array>
#include <functional>
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

private:
    void RefreshWorkList();
    bool SaveCurrentWork();
    bool SaveCurrentWorkForVerification(
        std::string& outWorkFileName,
        std::string& outErrorMessage);
    bool LoadSelectedWork();
    bool CopySelectedWorkToWorkingFile();
    bool DuplicateSelectedWork();
    bool DeleteSelectedWork();
    bool IsSelectedWorkVerified() const;
    const std::string* FindSelectedWorkFileName() const;

    DebugEditorContext& mContext;
    UGCWorkFileService mFileService;
    std::function<void()> mReloadSelectedWork;
    std::array<char, 96> mWorkName{"新しいステージ"};
    std::vector<std::string> mWorkFileNames;
    std::string mSaveErrorMessage;
    int mSelectedWorkIndex = -1;
    bool mHasLoadedWorkList = false;
    bool mShouldRefreshWorkList = false;
};
