#include "gfx/debug/DebugEditorLayoutController.h"

#include "gfx/debug/DebugEditorLayout.h"
#include "imgui.h"

#include <algorithm>

DebugEditorLayoutController::DebugEditorLayoutController(
    DebugEditorContext& context)
    : mContext(context)
{
}

void DebugEditorLayoutController::Resolve(DebugEditorSection section)
{
    constexpr float minimumRightPanelWidth = 280.0f;
    constexpr float minimumGameViewportWidth = 320.0f;
    constexpr float minimumAssetBrowserHeight = 150.0f;
    constexpr float minimumGameViewportHeight = 180.0f;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const bool showsHierarchy = section == DebugEditorSection::Stage;
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

void DebugEditorLayoutController::DrawResizeHandles(DebugEditorSection section)
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
    const bool showsHierarchy = section == DebugEditorSection::Stage;
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

    Resolve(section);
}

const char* DebugEditorLayoutController::ResolveToolPanelTitle(DebugEditorSection section) const
{
    switch (section) {
    case DebugEditorSection::BasicInfo:
        return "基本情報";
    case DebugEditorSection::Parameters:
        return "パラメータ調整";
    case DebugEditorSection::Particles:
        return "パーティクル";
    case DebugEditorSection::Sequences:
        return "演出エディタ";
    case DebugEditorSection::Tutorials:
        return "チュートリアル";
    case DebugEditorSection::Stage:
        return "ステージエディタ";
    case DebugEditorSection::UserInterface:
        return "UI調整";
    }

    return "エディタ";
}

