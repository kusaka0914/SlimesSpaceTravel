#pragma once

#include "gfx/debug/ugc/UGCWorkController.h"
#include "gfx/debug/ugc/UGCWorkState.h"

#include <functional>
#include <string>

struct DebugEditorContext;
class UGCEditorMenuState;
class UGCEditorToolState;

class UGCWorkPanel {
public:
    UGCWorkPanel(
        DebugEditorContext& context,
        UGCEditorMenuState& menuState,
        UGCEditorToolState& toolState,
        std::function<void()> reloadSelectedWork);

    void RequestManagementOpen();
    bool IsManagementOpen() const;
    void DrawManagement();
    void DrawBrowser();
    void StartVerification();
    bool CompleteVerification(const std::string& workFileName);
    bool HasUnsavedChanges() const;

private:
    void DrawCurrentWorkSaveControls(std::string& outStatusMessage);
    void DrawSavedWorkList(float listHeight);
    void DrawManagementDeleteConfirmation(std::string& outStatusMessage);
    bool DrawNewWorkConfirmation(std::string& outStatusMessage);

    DebugEditorContext& mContext;
    UGCEditorMenuState& mMenuState;
    UGCEditorToolState& mToolState;
    UGCWorkState mState;
    UGCWorkController mController;
};
