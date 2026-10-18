#include "editor/tools/parameters/ParameterDebugPanel.h"

#include "editor/tools/camera/CameraDebugPanel.h"
#include "imgui.h"

ParameterDebugPanel::ParameterDebugPanel(DebugEditorContext& context, CameraDebugPanel& cameraPanel)
    : DebugPanel(context),
      mCameraPanel(cameraPanel),
      mPlayerPanel(context),
      mEnemyPresetPanel(context),
      mEnemyPanel(context, mEnemyPresetPanel)
{
}

void ParameterDebugPanel::Draw()
{
    const char* menus[] = {"プレイヤー", "敵", "カメラ"};

    ImGui::BeginChild("ParameterEditorLeft", ImVec2(160, 0), true);

    for (int index = 0; index < IM_ARRAYSIZE(menus); ++index) {
        if (ImGui::Selectable(menus[index], mSelectedMenu == index)) {
            mSelectedMenu = index;
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("ParameterEditorRight", ImVec2(0, 0), true);

    switch (mSelectedMenu) {
    case 0:
        mPlayerPanel.Draw();
        break;
    case 1:
        mEnemyPanel.Draw();
        break;
    case 2:
        mCameraPanel.Draw();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}
