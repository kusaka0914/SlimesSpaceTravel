#include "gfx/debug/panels/SequenceEditorWorkspacePanel.h"

#include "gfx/debug/panels/CameraDebugPanel.h"
#include "gfx/debug/panels/EndingRollDebugPanel.h"
#include "gfx/debug/panels/SequenceDebugPanel.h"
#include "gfx/debug/panels/StorybookDebugPanel.h"
#include "gfx/debug/panels/StarCollectionDebugPanel.h"
#include "imgui.h"

#include <algorithm>

SequenceEditorWorkspacePanel::SequenceEditorWorkspacePanel(
    CameraDebugPanel& cameraPanel,
    SequenceDebugPanel& sequencePanel,
    EndingRollDebugPanel& endingRollPanel,
    StorybookDebugPanel& storybookPanel,
    StarCollectionDebugPanel& starCollectionPanel)
    : mCameraPanel(cameraPanel),
      mSequencePanel(sequencePanel),
      mEndingRollPanel(endingRollPanel),
      mStorybookPanel(storybookPanel),
      mStarCollectionPanel(starCollectionPanel)
{
}

int SequenceEditorWorkspacePanel::GetSelectedMenuIndex() const
{
    return mSelectedSequenceEditorMenu;
}

void SequenceEditorWorkspacePanel::SetSelectedMenuIndex(int menuIndex)
{
    mSelectedSequenceEditorMenu = std::clamp(menuIndex, 0, 4);
}

void SequenceEditorWorkspacePanel::Draw()
{
    constexpr const char* menus[] = {
        "演出シーケンス",
        "カメラシーケンス",
        "星獲得",
        "エンドロール",
        "絵本演出",
    };

    ImGui::BeginChild("SequenceEditorLeft", ImVec2(160.0f, 0.0f), true);

    for (int menuIndex = 0; menuIndex < IM_ARRAYSIZE(menus); ++menuIndex) {
        if (ImGui::Selectable(menus[menuIndex], mSelectedSequenceEditorMenu == menuIndex)) {
            mSelectedSequenceEditorMenu = menuIndex;
        }
    }

    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("SequenceEditorRight", ImVec2(0.0f, 0.0f), true);

    switch (mSelectedSequenceEditorMenu) {
    case 0:
        mSequencePanel.Draw();
        break;
    case 1:
        mCameraPanel.DrawCinematicSequenceEditor();
        break;
    case 2:
        mStarCollectionPanel.Draw();
        break;
    case 3:
        mEndingRollPanel.Draw();
        break;
    case 4:
        mStorybookPanel.Draw();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}
