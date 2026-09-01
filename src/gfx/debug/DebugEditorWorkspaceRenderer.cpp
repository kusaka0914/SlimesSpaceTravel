#include <GL/glew.h>

#include "gfx/debug/DebugEditorWorkspaceRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Star.h"
#include "gfx/debug/DebugBuildRestartPanel.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/DebugEditorLayoutController.h"
#include "gfx/debug/DebugGameViewportRenderer.h"
#include "gfx/debug/panels/AssetBrowserPanel.h"
#include "gfx/debug/panels/BasicInfoDebugPanel.h"
#include "gfx/debug/panels/ParameterDebugPanel.h"
#include "gfx/debug/panels/ParticleEffectDebugPanel.h"
#include "gfx/debug/panels/PerformanceDebugPanel.h"
#include "gfx/debug/panels/SequenceEditorWorkspacePanel.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageEditorPanel.h"
#include "gfx/debug/panels/TutorialDebugPanel.h"
#include "gfx/debug/panels/UIDebugPanel.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageSelectionOverlay.h"
#include "imgui.h"

#include <algorithm>
#include <string>

DebugEditorWorkspaceRenderer::DebugEditorWorkspaceRenderer(
    const DebugEditorWorkspaceDependencies& dependencies)
    : mContext(dependencies.context),
      mLayoutController(dependencies.layoutController),
      mGameViewportRenderer(dependencies.gameViewportRenderer),
      mBasicInfoPanel(dependencies.basicInfoPanel),
      mUIPanel(dependencies.uiPanel),
      mParameterPanel(dependencies.parameterPanel),
      mParticleEffectPanel(dependencies.particleEffectPanel),
      mSequenceEditorPanel(dependencies.sequenceEditorPanel),
      mTutorialPanel(dependencies.tutorialPanel),
      mAssetBrowserPanel(dependencies.assetBrowserPanel),
      mStageAddActorPanel(dependencies.stageAddActorPanel),
      mSelectionController(dependencies.selectionController),
      mEditCommandController(dependencies.editCommandController),
      mStageEditShortcutHandler(
          dependencies.context,
          dependencies.selectionController,
          dependencies.editCommandController),
      mStageEditorPanel(dependencies.stageEditorPanel),
      mGizmoController(dependencies.gizmoController)
{
}

DebugEditorShellSessionState
DebugEditorWorkspaceRenderer::CaptureShellState() const
{
    return {
        .activeSectionIndex = static_cast<int>(mActiveSection),
        .sequenceEditorMenuIndex = mSequenceEditorPanel.GetSelectedMenuIndex(),
    };
}

void DebugEditorWorkspaceRenderer::ApplyShellState(
    const DebugEditorShellSessionState& shellState)
{
    constexpr int firstSectionIndex =
        static_cast<int>(DebugEditorSection::BasicInfo);
    constexpr int lastSectionIndex =
        static_cast<int>(DebugEditorSection::UserInterface);
    mActiveSection = static_cast<DebugEditorSection>(std::clamp(
        shellState.activeSectionIndex,
        firstSectionIndex,
        lastSectionIndex));
    mSequenceEditorPanel.SetSelectedMenuIndex(
        shellState.sequenceEditorMenuIndex);
    mShouldSelectRestoredSection = true;
}

bool DebugEditorWorkspaceRenderer::IsUserInterfaceSectionActive() const
{
    return mActiveSection == DebugEditorSection::UserInterface;
}

void DebugEditorWorkspaceRenderer::Draw(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool wasStageSection =
        mActiveSection == DebugEditorSection::Stage;
    const float initialTopBarHeight =
        DebugEditorLayout::CalculateTopBarHeight(wasStageSection);
    ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x, initialTopBarHeight),
        ImGuiCond_Always);

    constexpr ImGuiWindowFlags topBarFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("デバッグ", nullptr, topBarFlags);

    DebugEditorSection activeSection = mActiveSection;
    bool requestedStageEditor = mStageEditorPanel.ConsumeRequestOpenMainTab();
    const auto restoredTabFlags = [this](DebugEditorSection section) {
        return mShouldSelectRestoredSection && mActiveSection == section
            ? ImGuiTabItemFlags_SetSelected
            : ImGuiTabItemFlags_None;
    };

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報", nullptr, restoredTabFlags(DebugEditorSection::BasicInfo))) {
            activeSection = DebugEditorSection::BasicInfo;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パラメータ調整", nullptr, restoredTabFlags(DebugEditorSection::Parameters))) {
            activeSection = DebugEditorSection::Parameters;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パーティクル", nullptr, restoredTabFlags(DebugEditorSection::Particles))) {
            activeSection = DebugEditorSection::Particles;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("演出エディタ", nullptr, restoredTabFlags(DebugEditorSection::Sequences))) {
            activeSection = DebugEditorSection::Sequences;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("チュートリアル", nullptr, restoredTabFlags(DebugEditorSection::Tutorials))) {
            activeSection = DebugEditorSection::Tutorials;
            ImGui::EndTabItem();
        }

        const ImGuiTabItemFlags stageEditorTabFlags =
            requestedStageEditor
                ? ImGuiTabItemFlags_SetSelected
                : restoredTabFlags(DebugEditorSection::Stage);

        if (ImGui::BeginTabItem("ステージエディタ", nullptr, stageEditorTabFlags)) {
            activeSection = DebugEditorSection::Stage;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI調整", nullptr, restoredTabFlags(DebugEditorSection::UserInterface))) {
            activeSection = DebugEditorSection::UserInterface;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    mShouldSelectRestoredSection = false;

    if (activeSection == DebugEditorSection::Stage) {
        ImGui::SetWindowSize(
            ImVec2(
                viewport->WorkSize.x,
                DebugEditorLayout::StageTopBarHeight),
            ImGuiCond_Always);
        mStageEditorPanel.DrawTopBar();
    }

    ImGui::End();
    mActiveSection = activeSection;
    mLayoutController.Resolve(activeSection);
    mGameViewportRenderer.Draw(
        activeSection,
        gameViewTexture,
        gameViewWidth,
        gameViewHeight);


    if (activeSection == DebugEditorSection::Stage) {
        mStageAddActorPanel.UpdatePlacement();
        const bool isPlacingActor = mStageAddActorPanel.IsPlacementActive();
        if (!isPlacingActor) {
            mSelectionController.SetBoxSelectionEnabled(true);
            mSelectionController.Update();
        }

        if (mSelectionController.ConsumeRequestOpenPlacement()) {
            mStageEditorPanel.RequestOpenPlacementTab();
        }

        if (!isPlacingActor) {
            mStageEditShortcutHandler.Update();
        }
        if (mEditCommandController.ConsumeRequestOpenPlacement()) {
            mStageEditorPanel.RequestOpenPlacementTab();
        }

        mSelectionController.ApplyEditorSelectionFlags();
        DrawStageSelectionOverlay(mSelectionController);
        if (!isPlacingActor) {
            mGizmoController.Update();
        }
        mStageEditorPanel.Draw();
    } else {
        DrawDockedToolPanel(activeSection);
    }

    DrawDockedAssetBrowser(activeSection);
    mLayoutController.DrawResizeHandles(activeSection);

}

void DebugEditorWorkspaceRenderer::DrawDockedToolPanel(DebugEditorSection section)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float panelWidth = mContext.layout.rightPanelWidth;
    const float workspaceTop =
        viewport->WorkPos.y +
        DebugEditorLayout::CalculateTopBarHeight(false);

    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x - panelWidth,
            workspaceTop),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(
            panelWidth,
            viewport->WorkPos.y + viewport->WorkSize.y - workspaceTop),
        ImGuiCond_Always);

    const std::string windowTitle =
        std::string(mLayoutController.ResolveToolPanelTitle(section)) + "###DebugToolPanel";
    ImGui::Begin(windowTitle.c_str(), nullptr, panelFlags);

    switch (section) {
    case DebugEditorSection::BasicInfo:
        mBasicInfoPanel.Draw();
        break;
    case DebugEditorSection::Parameters:
        mParameterPanel.Draw();
        break;
    case DebugEditorSection::Particles:
        mParticleEffectPanel.Draw();
        break;
    case DebugEditorSection::Sequences:
        mSequenceEditorPanel.Draw();
        break;
    case DebugEditorSection::Tutorials:
        mTutorialPanel.Draw();
        break;
    case DebugEditorSection::UserInterface:
        mUIPanel.Draw();
        break;
    case DebugEditorSection::Stage:
        break;
    }

    ImGui::End();
}

void DebugEditorWorkspaceRenderer::DrawDockedAssetBrowser(DebugEditorSection section)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool showsHierarchy = section == DebugEditorSection::Stage;
    const float hierarchyWidth = showsHierarchy
        ? DebugEditorLayout::CalculateHierarchyWidth(viewport->WorkSize.x)
        : 0.0f;
    const float toolPanelWidth = mContext.layout.rightPanelWidth;
    const float assetBrowserHeight = mContext.layout.assetBrowserHeight;
    const float browserWidth =
        std::max(
            200.0f,
            viewport->WorkSize.x - hierarchyWidth - toolPanelWidth);

    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + hierarchyWidth,
            viewport->WorkPos.y + viewport->WorkSize.y - assetBrowserHeight),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(browserWidth, assetBrowserHeight),
        ImGuiCond_Always);
    ImGui::Begin("アセット###DebugAssetBrowser", nullptr, panelFlags);
    mAssetBrowserPanel.Draw();
    ImGui::End();
}



