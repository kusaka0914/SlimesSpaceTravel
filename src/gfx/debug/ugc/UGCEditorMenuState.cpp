#include "gfx/debug/ugc/UGCEditorMenuState.h"

void UGCEditorMenuState::RequestMenuOpen()
{
    mShouldOpenMenu = true;
}

bool UGCEditorMenuState::ConsumeMenuOpenRequest()
{
    const bool wasRequested = mShouldOpenMenu;
    mShouldOpenMenu = false;
    return wasRequested;
}

void UGCEditorMenuState::RequestWorkManagementOpen()
{
    mShouldOpenWorkManagement = true;
}

bool UGCEditorMenuState::HasWorkManagementOpenRequest() const
{
    return mShouldOpenWorkManagement;
}

bool UGCEditorMenuState::ConsumeWorkManagementOpenRequest()
{
    const bool wasRequested = mShouldOpenWorkManagement;
    mShouldOpenWorkManagement = false;
    return wasRequested;
}
