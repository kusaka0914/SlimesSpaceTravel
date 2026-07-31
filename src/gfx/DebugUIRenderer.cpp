#include "DebugUIRenderer.h"

#include "imgui.h"

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer},
      mPerformancePanel(mContext),
      mCameraPanel(mContext),
      mUIPanel(mContext),
      mParameterPanel(mContext),
      mParticleEffectPanel(mContext),
      mSequencePanel(mContext),
      mTutorialPanel(mContext),
      mStageAddActorPanel(mContext),
      mStagePlanetPanel(mContext),
      mSelectionController(mContext),
      mStagePlacementPanel(
          mContext,
          mSelectionController,
          [this]() { mEditCommandController.PushUndo(); }),
      mEditCommandController(mContext, mSelectionController),
      mStageDeleteActorPanel(mContext, mEditCommandController),
      mStageEditorPanel(
          mContext,
          mStageAddActorPanel,
          mStagePlanetPanel,
          mStagePlacementPanel,
          mStageDeleteActorPanel,
          mSelectionController),
      mGizmoController(
          mContext,
          mSelectionController,
          [this]() { mEditCommandController.PushUndo(); },
          [this]() { mStagePlacementPanel.Save(); })
{
}

void DebugUIRenderer::Draw()
{
    ImGui::Begin("デバッグ");

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {
            mPerformancePanel.Draw();
            mCameraPanel.Draw();
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
            mSequencePanel.Draw();
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

        if (ImGui::BeginTabItem(
                "ステージエディタ",
                nullptr,
                stageEditorTabFlags)) {
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
