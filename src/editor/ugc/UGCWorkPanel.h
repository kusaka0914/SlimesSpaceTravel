#pragma once

#include "editor/ugc/UGCWorkController.h"
#include "editor/ugc/UGCWorkState.h"

#include <functional>
#include <string>

struct DebugEditorContext;
class UGCEditorMenuState;
class UGCEditorToolState;

// 作品管理・作品ブラウザのUIを描画し、作品操作用の状態とコントローラを所有する。contextと編集状態は所有しない。
class UGCWorkPanel {
public:
    UGCWorkPanel(DebugEditorContext& context, UGCEditorMenuState& menuState, UGCEditorToolState& toolState,
                 std::function<void()> reloadSelectedWork);

    void RequestManagementOpen();

    // ImGuiフレーム内でDrawManagementと同じID階層から呼ぶ。表示要求の保留中もtrueを返す。
    bool IsManagementOpen() const;
    void DrawManagement();
    void DrawBrowser();

    // 保存済み作品は上書きし、未保存作品は現在の作品名入力で保存する。失敗した場合はfalse。
    bool SaveCurrentWork();
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
