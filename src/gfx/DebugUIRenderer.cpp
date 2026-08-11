#include "DebugUIRenderer.h"

#include "Game.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "imgui.h"

#include <algorithm>
#include <string>

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer, &mAssetCatalog},
      mPerformancePanel(mContext),
      mCameraPanel(mContext),
      mUIPanel(mContext),
      mParameterPanel(mContext, mCameraPanel),
      mParticleEffectPanel(mContext),
      mSequencePanel(mContext),
      mTutorialPanel(mContext),
      mAssetBrowserPanel(mContext),
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
    mStageAddActorPanel.SetSelectionController(&mSelectionController);
    mStageAddActorPanel.SetPushUndoCallback(
        [this]() { mEditCommandController.PushUndo(); });
}

void DebugUIRenderer::Draw(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool wasStageSection =
        mActiveSection == EditorSection::Stage;
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

    EditorSection activeSection = mActiveSection;
    bool requestedStageEditor = mStageEditorPanel.ConsumeRequestOpenMainTab();

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報")) {
            activeSection = EditorSection::BasicInfo;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パラメータ調整")) {
            activeSection = EditorSection::Parameters;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パーティクル")) {
            activeSection = EditorSection::Particles;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("演出エディタ")) {
            activeSection = EditorSection::Sequences;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("チュートリアル")) {
            activeSection = EditorSection::Tutorials;
            ImGui::EndTabItem();
        }

        const ImGuiTabItemFlags stageEditorTabFlags =
            requestedStageEditor ? ImGuiTabItemFlags_SetSelected : 0;

        if (ImGui::BeginTabItem("ステージエディタ", nullptr, stageEditorTabFlags)) {
            activeSection = EditorSection::Stage;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI調整")) {
            activeSection = EditorSection::UserInterface;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    if (activeSection == EditorSection::Stage) {
        ImGui::SetWindowSize(
            ImVec2(
                viewport->WorkSize.x,
                DebugEditorLayout::StageTopBarHeight),
            ImGuiCond_Always);
        mStageEditorPanel.DrawTopBar();
    }

    ImGui::End();
    mActiveSection = activeSection;
    ResolveResizableLayout(activeSection);
    DrawGameViewport(
        activeSection,
        gameViewTexture,
        gameViewWidth,
        gameViewHeight);

    if (activeSection == EditorSection::Stage) {
        mStageAddActorPanel.UpdatePlacement();
        const bool isPlacingActor = mStageAddActorPanel.IsPlacementActive();
        if (!isPlacingActor) {
            mSelectionController.Update();
        }

        if (mSelectionController.ConsumeRequestOpenPlacement()) {
            mStageEditorPanel.RequestOpenPlacementTab();
        }

        if (!isPlacingActor) {
            mEditCommandController.UpdateShortcuts();
        }
        if (mEditCommandController.ConsumeRequestOpenPlacement()) {
            mStageEditorPanel.RequestOpenPlacementTab();
        }

        mSelectionController.ApplyEditorSelectionFlags();
        mSelectionController.DrawBoxSelectionRect();
        if (!isPlacingActor) {
            mGizmoController.Update();
        }
        mStageEditorPanel.Draw();
    } else {
        DrawDockedToolPanel(activeSection);
    }

    DrawDockedAssetBrowser(activeSection);
    DrawLayoutResizeHandles(activeSection);
}

void DebugUIRenderer::DrawDockedToolPanel(EditorSection section)
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
        std::string(ResolveToolPanelTitle(section)) + "###DebugToolPanel";
    ImGui::Begin(windowTitle.c_str(), nullptr, panelFlags);

    switch (section) {
    case EditorSection::BasicInfo:
        DrawBasicInfoTab();
        break;
    case EditorSection::Parameters:
        mParameterPanel.Draw();
        break;
    case EditorSection::Particles:
        mParticleEffectPanel.Draw();
        break;
    case EditorSection::Sequences:
        DrawSequenceEditorTab();
        break;
    case EditorSection::Tutorials:
        mTutorialPanel.Draw();
        break;
    case EditorSection::UserInterface:
        mUIPanel.Draw();
        break;
    case EditorSection::Stage:
        break;
    }

    ImGui::End();
}

void DebugUIRenderer::DrawDockedAssetBrowser(EditorSection section)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool showsHierarchy = section == EditorSection::Stage;
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

void DebugUIRenderer::DrawGameViewport(
    EditorSection section,
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float hierarchyWidth = section == EditorSection::Stage
        ? DebugEditorLayout::CalculateHierarchyWidth(viewport->WorkSize.x)
        : 0.0f;
    const float toolPanelWidth = mContext.layout.rightPanelWidth;
    const float assetBrowserHeight = mContext.layout.assetBrowserHeight;

    const ImVec2 availableMin(
        viewport->WorkPos.x + hierarchyWidth,
        viewport->WorkPos.y +
            DebugEditorLayout::CalculateTopBarHeight(
                section == EditorSection::Stage));
    const ImVec2 availableMax(
        viewport->WorkPos.x + viewport->WorkSize.x - toolPanelWidth,
        viewport->WorkPos.y + viewport->WorkSize.y - assetBrowserHeight);
    const float availableWidth =
        std::max(1.0f, availableMax.x - availableMin.x);
    DrawGameViewportToolbar(
        availableMin,
        availableWidth,
        section == EditorSection::Stage);

    const ImVec2 gameContentMin(
        availableMin.x,
        availableMin.y + DebugEditorLayout::GameViewportToolbarHeight);
    const float gameContentHeight =
        std::max(1.0f, availableMax.y - gameContentMin.y);

    ImDrawList* background = ImGui::GetBackgroundDrawList();
    background->AddRectFilled(
        gameContentMin,
        availableMax,
        IM_COL32(20, 20, 24, 255));

    mContext.gameViewport = {};
    if (gameViewTexture == 0 || gameViewWidth <= 0 || gameViewHeight <= 0) {
        background->AddText(
            ImVec2(gameContentMin.x + 12.0f, gameContentMin.y + 10.0f),
            IM_COL32(255, 255, 255, 145),
            "ゲーム画面を取得できません");
        return;
    }

    const float sourceAspect =
        static_cast<float>(gameViewWidth) /
        static_cast<float>(gameViewHeight);
    float imageWidth = availableWidth;
    float imageHeight = imageWidth / sourceAspect;
    if (imageHeight > gameContentHeight) {
        imageHeight = gameContentHeight;
        imageWidth = imageHeight * sourceAspect;
    }

    const ImVec2 imageMin(
        gameContentMin.x + (availableWidth - imageWidth) * 0.5f,
        gameContentMin.y + (gameContentHeight - imageHeight) * 0.5f);
    const ImVec2 imageMax(
        imageMin.x + imageWidth,
        imageMin.y + imageHeight);
    background->AddImage(
        static_cast<ImTextureID>(gameViewTexture),
        imageMin,
        imageMax,
        ImVec2(0.0f, 1.0f),
        ImVec2(1.0f, 0.0f));

    mContext.gameViewport.x = imageMin.x;
    mContext.gameViewport.y = imageMin.y;
    mContext.gameViewport.width = imageWidth;
    mContext.gameViewport.height = imageHeight;
    mContext.gameViewport.sourceWidth = gameViewWidth;
    mContext.gameViewport.sourceHeight = gameViewHeight;
}

void DebugUIRenderer::DrawGameViewportToolbar(
    const ImVec2& toolbarMin,
    float toolbarWidth,
    bool showGizmoTranslationSpace)
{
    constexpr ImGuiWindowFlags toolbarFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::SetNextWindowPos(toolbarMin, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(
            toolbarWidth,
            DebugEditorLayout::GameViewportToolbarHeight),
        ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(7.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 0.0f));
    ImGui::Begin("ビュー切り替え###DebugGameViewportToolbar", nullptr, toolbarFlags);

    const bool isSceneView =
        mContext.game && mContext.game->GetIsFreeCameraMode();

    if (ImGui::Selectable(
            "シーン",
            isSceneView,
            0,
            ImVec2(90.0f, 26.0f)) &&
        mContext.game) {
        mContext.game->SetFreeCameraMode(true);
    }

    ImGui::SameLine();
    if (ImGui::Selectable(
            "ゲーム",
            !isSceneView,
            0,
            ImVec2(90.0f, 26.0f)) &&
        mContext.game) {
        mContext.game->SetFreeCameraMode(false);
    }

    if (showGizmoTranslationSpace) {
        const bool usesPlanetSurfaceTranslation =
            mGizmoController.GetTranslationSpace() ==
            StageGizmoController::TranslationSpace::PlanetSurface;
        const bool canChangeTranslationSpace =
            !mGizmoController.IsUsingTransformGizmo();

        ImGui::SameLine();
        ImGui::TextDisabled("移動軸:");

        ImGui::SameLine();
        ImGui::BeginDisabled(!canChangeTranslationSpace);
        if (ImGui::Selectable(
                "惑星表面",
                usesPlanetSurfaceTranslation,
                0,
                ImVec2(100.0f, 26.0f))) {
            mGizmoController.SetTranslationSpace(
                StageGizmoController::TranslationSpace::PlanetSurface);
        }

        ImGui::SameLine();
        if (ImGui::Selectable(
                "ワールド",
                !usesPlanetSurfaceTranslation,
                0,
                ImVec2(90.0f, 26.0f))) {
            mGizmoController.SetTranslationSpace(
                StageGizmoController::TranslationSpace::World);
        }
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(
                "惑星表面: 選択物の上・前・横方向\n"
                "ワールド: XYZ固定方向\n"
                "複数選択の移動は常にワールド基準です");
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled(
        isSceneView
            ? "フリーカメラ（Lキーでも切り替え）"
            : "通常カメラ（Lキーでも切り替え）");

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void DebugUIRenderer::ResolveResizableLayout(EditorSection section)
{
    constexpr float minimumRightPanelWidth = 280.0f;
    constexpr float minimumGameViewportWidth = 320.0f;
    constexpr float minimumAssetBrowserHeight = 150.0f;
    constexpr float minimumGameViewportHeight = 180.0f;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool showsHierarchy = section == EditorSection::Stage;
    const float hierarchyWidth = showsHierarchy
        ? DebugEditorLayout::CalculateHierarchyWidth(viewport->WorkSize.x)
        : 0.0f;
    const float availableWorkspaceWidth =
        std::max(1.0f, viewport->WorkSize.x - hierarchyWidth);
    const float resolvedMinimumRightPanelWidth =
        std::min(minimumRightPanelWidth, availableWorkspaceWidth);
    const float maximumRightPanelWidth = std::max(
        resolvedMinimumRightPanelWidth,
        availableWorkspaceWidth - minimumGameViewportWidth);

    if (mContext.layout.rightPanelWidth <= 0.0f) {
        mContext.layout.rightPanelWidth =
            DebugEditorLayout::CalculateToolPanelWidth(
                viewport->WorkSize.x);
    }
    mContext.layout.rightPanelWidth = std::clamp(
        mContext.layout.rightPanelWidth,
        resolvedMinimumRightPanelWidth,
        maximumRightPanelWidth);

    const float topBarHeight =
        DebugEditorLayout::CalculateTopBarHeight(showsHierarchy);
    const float availableWorkspaceHeight = std::max(
        1.0f,
        viewport->WorkSize.y - topBarHeight -
            DebugEditorLayout::GameViewportToolbarHeight);
    const float resolvedMinimumAssetBrowserHeight =
        std::min(minimumAssetBrowserHeight, availableWorkspaceHeight);
    const float maximumAssetBrowserHeight = std::max(
        resolvedMinimumAssetBrowserHeight,
        availableWorkspaceHeight - minimumGameViewportHeight);

    if (mContext.layout.assetBrowserHeight <= 0.0f) {
        mContext.layout.assetBrowserHeight =
            DebugEditorLayout::CalculateAssetBrowserHeight(
                viewport->WorkSize.y);
    }
    mContext.layout.assetBrowserHeight = std::clamp(
        mContext.layout.assetBrowserHeight,
        resolvedMinimumAssetBrowserHeight,
        maximumAssetBrowserHeight);
}

void DebugUIRenderer::DrawLayoutResizeHandles(EditorSection section)
{
    constexpr float resizeHandleThickness = 8.0f;
    constexpr ImGuiWindowFlags resizeHandleFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoNav |
        ImGuiWindowFlags_NoFocusOnAppearing;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool showsHierarchy = section == EditorSection::Stage;
    const float hierarchyWidth = showsHierarchy
        ? DebugEditorLayout::CalculateHierarchyWidth(viewport->WorkSize.x)
        : 0.0f;
    const float workspaceTop =
        viewport->WorkPos.y +
        DebugEditorLayout::CalculateTopBarHeight(showsHierarchy);
    const float workspaceBottom =
        viewport->WorkPos.y + viewport->WorkSize.y;
    const float rightPanelBoundaryX =
        viewport->WorkPos.x + viewport->WorkSize.x -
        mContext.layout.rightPanelWidth;
    const float assetBrowserBoundaryY =
        workspaceBottom - mContext.layout.assetBrowserHeight;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::SetNextWindowPos(
        ImVec2(
            rightPanelBoundaryX - resizeHandleThickness * 0.5f,
            workspaceTop),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(
            resizeHandleThickness,
            std::max(1.0f, workspaceBottom - workspaceTop)),
        ImGuiCond_Always);
    ImGui::Begin(
        "##DebugRightPanelResizeHandle",
        nullptr,
        resizeHandleFlags);
    ImGui::InvisibleButton(
        "##RightPanelResize",
        ImGui::GetContentRegionAvail());
    const bool isRightHandleHovered = ImGui::IsItemHovered();
    const bool isRightHandleActive = ImGui::IsItemActive();
    if (isRightHandleHovered || isRightHandleActive) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    if (isRightHandleActive) {
        mContext.layout.rightPanelWidth -= ImGui::GetIO().MouseDelta.x;
    }
    ImGui::End();

    const float assetBrowserLeft =
        viewport->WorkPos.x + hierarchyWidth;
    const float assetBrowserWidth = std::max(
        1.0f,
        rightPanelBoundaryX - assetBrowserLeft);
    ImGui::SetNextWindowPos(
        ImVec2(
            assetBrowserLeft,
            assetBrowserBoundaryY - resizeHandleThickness * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(assetBrowserWidth, resizeHandleThickness),
        ImGuiCond_Always);
    ImGui::Begin(
        "##DebugAssetBrowserResizeHandle",
        nullptr,
        resizeHandleFlags);
    ImGui::InvisibleButton(
        "##AssetBrowserResize",
        ImGui::GetContentRegionAvail());
    const bool isAssetHandleHovered = ImGui::IsItemHovered();
    const bool isAssetHandleActive = ImGui::IsItemActive();
    if (isAssetHandleHovered || isAssetHandleActive) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
    if (isAssetHandleActive) {
        mContext.layout.assetBrowserHeight -= ImGui::GetIO().MouseDelta.y;
    }
    ImGui::End();

    ImGui::PopStyleVar();

    ImDrawList* foreground = ImGui::GetForegroundDrawList();
    const ImU32 normalLineColor = IM_COL32(74, 78, 88, 255);
    const ImU32 activeLineColor = IM_COL32(90, 150, 235, 255);
    foreground->AddLine(
        ImVec2(rightPanelBoundaryX, workspaceTop),
        ImVec2(rightPanelBoundaryX, workspaceBottom),
        isRightHandleHovered || isRightHandleActive
            ? activeLineColor
            : normalLineColor,
        isRightHandleActive ? 2.0f : 1.0f);
    foreground->AddLine(
        ImVec2(assetBrowserLeft, assetBrowserBoundaryY),
        ImVec2(rightPanelBoundaryX, assetBrowserBoundaryY),
        isAssetHandleHovered || isAssetHandleActive
            ? activeLineColor
            : normalLineColor,
        isAssetHandleActive ? 2.0f : 1.0f);

    ResolveResizableLayout(section);
}

const char* DebugUIRenderer::ResolveToolPanelTitle(EditorSection section) const
{
    switch (section) {
    case EditorSection::BasicInfo:
        return "基本情報";
    case EditorSection::Parameters:
        return "パラメータ調整";
    case EditorSection::Particles:
        return "パーティクル";
    case EditorSection::Sequences:
        return "演出エディタ";
    case EditorSection::Tutorials:
        return "チュートリアル";
    case EditorSection::Stage:
        return "ステージエディタ";
    case EditorSection::UserInterface:
        return "UI調整";
    }

    return "エディタ";
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
