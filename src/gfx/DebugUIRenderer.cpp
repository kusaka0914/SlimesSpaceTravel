#include "DebugUIRenderer.h"

#include "imgui.h"

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer},
      mPerformancePanel(mContext),
      mCameraPanel(mContext),
      mUIPanel(mContext),
      mParameterPanel(mContext, mCameraPanel),
      mParticleEffectPanel(mContext),
      mSequencePanel(mContext),
      mTutorialPanel(mContext),
      mStageAddActorPanel(mContext),
      mStagePlanetPanel(mContext),
      mSelectionController(mContext),
      mStagePlacementPanel(mContext, mSelectionController, [this]() { mEditCommandController.PushUndo(); }),
      mEditCommandController(mContext, mSelectionController),
      mStageDeleteActorPanel(mContext, mEditCommandController),
      mStageEditorPanel(mContext, mStageAddActorPanel, mStagePlanetPanel, mStagePlacementPanel, mStageDeleteActorPanel,
                        mSelectionController),
      mGizmoController(
          mContext, mSelectionController, [this]() { mEditCommandController.PushUndo(); },
          [this]() { mStagePlacementPanel.Save(); })
{
}

void DebugUIRenderer::Draw()
{
    ImGui::Begin("デバッグ");

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {
            DrawBasicInfoTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パラメータ調整")) {
            mParameterPanel.Draw();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パーティクル")) {
            mParticleEffectPanel.Draw();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("演出エディタ")) {
            DrawSequenceEditorTab();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("チュートリアル")) {
            mTutorialPanel.Draw();
            ImGui::EndTabItem();
        }

        ImGuiTabItemFlags stageEditorTabFlags = 0;

        if (mStageEditorPanel.ConsumeRequestOpenMainTab()) {
            stageEditorTabFlags |= ImGuiTabItemFlags_SetSelected;
        }

        if (ImGui::BeginTabItem("ステージエディタ", nullptr, stageEditorTabFlags)) {
            mSelectionController.Update();

            if (mSelectionController.ConsumeRequestOpenPlacement()) {
                mStageEditorPanel.RequestOpenPlacementTab();
            }

            mEditCommandController.UpdateShortcuts();

            if (mEditCommandController.ConsumeRequestOpenPlacement()) {
                mStageEditorPanel.RequestOpenPlacementTab();
            }

            mSelectionController.ApplyEditorSelectionFlags();
            mSelectionController.DrawBoxSelectionRect();
            mGizmoController.Update();

            mStageEditorPanel.Draw();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI調整")) {
            mUIPanel.Draw();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void DebugUIRenderer::DrawBasicInfoTab()
{
    ImGui::BeginChild("BasicInfoLeft", ImVec2(160.0f, 0.0f), true);
    ImGui::Selectable("パフォーマンス", true);
    ImGui::EndChild();
    ImGui::SameLine();

    ImGui::BeginChild("BasicInfoRight", ImVec2(0.0f, 0.0f), true);

    mPerformancePanel.Draw();

    ImGui::EndChild();
}

void DebugUIRenderer::DrawSequenceEditorTab()
{
    constexpr const char* menus[] = {
        "演出シーケンス",
        "カメラシーケンス",
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
    default:
        break;
    }

    ImGui::EndChild();
}
