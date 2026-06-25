#include "gfx/debug/panels/StageEditorPanel.h"

#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"

#include "imgui.h"

StageEditorPanel::StageEditorPanel(DebugEditorContext& context, StageAddActorPanel& addActorPanel,
                                   StagePlanetPanel& planetPanel, StagePlacementPanel& placementPanel,
                                   StageDeleteActorPanel& deleteActorPanel)
    : DebugPanel(context),
      mAddActorPanel(addActorPanel),
      mPlanetPanel(planetPanel),
      mPlacementPanel(placementPanel),
      mDeleteActorPanel(deleteActorPanel)
{
}

void StageEditorPanel::Draw()
{
    const char* menus[] = {"追加", "配置", "削除"};

    ImGui::BeginChild("StageEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mSelectedMenu == i)) {
            mSelectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する", ImVec2(-1, 0))) {
        mPlanetPanel.Save();
        mPlacementPanel.Save();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("StageEditorRight", ImVec2(0, 0), true);

    switch (mSelectedMenu) {
    case 0:
        mAddActorPanel.Draw();
        break;
    case 1:
        mPlanetPanel.Draw();
        mPlacementPanel.Draw();
        break;
    case 2:
        mDeleteActorPanel.Draw();
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void StageEditorPanel::RequestOpenPlacementTab()
{
    mRequestOpenMainTab = true;
    mSelectedMenu = 1;
    mPlacementPanel.RequestOpenPickedActorPlacement();
}

bool StageEditorPanel::ConsumeRequestOpenMainTab()
{
    const bool result = mRequestOpenMainTab;
    mRequestOpenMainTab = false;
    return result;
}