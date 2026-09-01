#pragma once

class UGCEditorMenuState {
public:
    void RequestMenuOpen();
    bool ConsumeMenuOpenRequest();
    void RequestWorkManagementOpen();
    bool HasWorkManagementOpenRequest() const;
    bool ConsumeWorkManagementOpenRequest();

private:
    bool mShouldOpenMenu = false;
    bool mShouldOpenWorkManagement = false;
};
