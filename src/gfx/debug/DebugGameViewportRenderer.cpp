#include <GL/glew.h>

#include "gfx/debug/DebugGameViewportRenderer.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/DebugBuildRestartPanel.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "system/CameraSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>

DebugGameViewportRenderer::DebugGameViewportRenderer(
    DebugEditorContext& context,
    StageSelectionController& selectionController,
    StageGizmoController& gizmoController,
    DebugBuildRestartPanel& buildRestartPanel)
    : mContext(context),
      mSelectionController(selectionController),
      mGizmoController(gizmoController),
      mBuildRestartPanel(buildRestartPanel)
{
}

void DebugGameViewportRenderer::Draw(
    DebugEditorSection section,
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float hierarchyWidth = section == DebugEditorSection::Stage
        ? DebugEditorLayout::CalculateHierarchyWidth(viewport->WorkSize.x)
        : 0.0f;
    const float toolPanelWidth = mContext.layout.rightPanelWidth;
    const float assetBrowserHeight = mContext.layout.assetBrowserHeight;

    const ImVec2 availableMin(
        viewport->WorkPos.x + hierarchyWidth,
        viewport->WorkPos.y +
            DebugEditorLayout::CalculateTopBarHeight(
                section == DebugEditorSection::Stage));
    const ImVec2 availableMax(
        viewport->WorkPos.x + viewport->WorkSize.x - toolPanelWidth,
        viewport->WorkPos.y + viewport->WorkSize.y - assetBrowserHeight);
    const float availableWidth =
        std::max(1.0f, availableMax.x - availableMin.x);
    DrawToolbar(
        availableMin,
        availableWidth,
        section == DebugEditorSection::Stage);

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

void DebugGameViewportRenderer::DrawToolbar(
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

    if (isSceneView) {
        ImGui::SameLine();
        Actor* selectedActor =
            mSelectionController.GetSingleSelectedActor();
        const bool canAlignCameraUp =
            selectedActor &&
            glm::dot(
                selectedActor->GetUpVec(),
                selectedActor->GetUpVec()) > 0.000001f;

        ImGui::BeginDisabled(!canAlignCameraUp);
        if (ImGui::Button("選択物の上方向に合わせる")) {
            AlignFreeCameraUpToSelectedActor();
        }
        ImGui::EndDisabled();

        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip(
                canAlignCameraUp
                    ? "カメラ位置と向きを維持し、上方向を選択物に合わせます"
                    : "オブジェクトを1つ選択してください");
        }
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

        ImGui::SameLine();
        ImGui::TextDisabled("惑星移動:");

        const bool movesBoundActors =
            mContext.planetMoveMode ==
            PlanetMoveMode::WithBoundActors;

        ImGui::SameLine();
        if (ImGui::Selectable(
                "所属物も",
                movesBoundActors,
                0,
                ImVec2(90.0f, 26.0f))) {
            mContext.planetMoveMode =
                PlanetMoveMode::WithBoundActors;
        }

        ImGui::SameLine();
        if (ImGui::Selectable(
                "惑星のみ",
                !movesBoundActors,
                0,
                ImVec2(90.0f, 26.0f))) {
            mContext.planetMoveMode =
                PlanetMoveMode::PlanetOnly;
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip(
                "所属物も: 惑星と所属する配置物・プレイヤーを一緒に移動\n"
                "惑星のみ: 配置物とプレイヤーを残して惑星本体だけを移動");
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled(
        isSceneView
            ? "フリーカメラ（Lキーでも切り替え）"
            : "通常カメラ（Lキーでも切り替え）");

    ImGui::SameLine();
    mBuildRestartPanel.Draw();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void DebugGameViewportRenderer::AlignFreeCameraUpToSelectedActor()
{
    if (!mContext.game || !mContext.game->GetIsFreeCameraMode()) {
        return;
    }

    Actor* selectedActor =
        mSelectionController.GetSingleSelectedActor();
    CameraSystem* cameraSystem =
        mContext.game->GetCameraSystem();
    if (!selectedActor || !cameraSystem) {
        return;
    }

    const glm::vec3 selectedActorUp =
        selectedActor->GetUpVec();
    if (glm::dot(selectedActorUp, selectedActorUp) <= 0.000001f) {
        return;
    }

    CameraPose cameraPose =
        cameraSystem->GetDebugCameraPose();
    cameraPose.up = glm::normalize(selectedActorUp);
    cameraSystem->SetDebugCameraPose(cameraPose);
}
