#pragma once

#include "gfx/debug/ugc/UGCWorkController.h"
#include "gfx/debug/ugc/UGCWorkState.h"

#include <functional>
#include <string>

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
    void DrawCurrentWorkSaveControls(std::string& outStatusMessage);
    void DrawSavedWorkList(float listHeight);
    void DrawManagementDeleteConfirmation(std::string& outStatusMessage);
    bool DrawNewWorkConfirmation(std::string& outStatusMessage);

    DebugEditorContext& mContext;
    UGCWorkState mState;
    UGCWorkController mController;
};
