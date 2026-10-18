#pragma once

// メニューと作品管理画面の表示要求を、消費されるまで保持する。
class UGCEditorMenuState {
public:
    void RequestMenuOpen();

    // 表示要求を返し、要求を解除する。
    bool ConsumeMenuOpenRequest();
    void RequestWorkManagementOpen();
    bool HasWorkManagementOpenRequest() const;

    // 表示要求を返し、要求を解除する。
    bool ConsumeWorkManagementOpenRequest();

private:
    bool mShouldOpenMenu = false;
    bool mShouldOpenWorkManagement = false;
};
