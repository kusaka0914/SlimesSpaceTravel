#include "DebugUIRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/session/EditorSessionState.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "system/CameraSystem.h"
#include "imgui.h"

#include <algorithm>
#include <string>
#include <unordered_set>

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
          [this]() {
              mStagePlanetPanel.SaveEditorAuthoredTransforms();
          })
{
    mStagePlanetPanel.SetSaveDependentActorTransformsCallback(
        [this]() { mStagePlacementPanel.SaveEditorAuthoredTransforms(); });
    mStageAddActorPanel.SetSelectionController(&mSelectionController);
    mStageAddActorPanel.SetPushUndoCallback(
        [this]() { mEditCommandController.PushUndo(); });
}

bool DebugUIRenderer::SaveEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    // Runtime movement is intentionally excluded. The planet panel also
    // invokes the registered callback that saves authored actor transforms.
    mStagePlanetPanel.SaveEditorAuthoredTransforms();

    return EditorSessionRepository::Save(
        filePath,
        CaptureEditorSessionState(),
        outErrorMessage);
}

bool DebugUIRenderer::RestoreEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{
    EditorSessionState sessionState;
    if (!EditorSessionRepository::Load(
            filePath,
            sessionState,
            outErrorMessage)) {
        return false;
    }

    if (!mContext.game) {
        outErrorMessage = "The game is not available while restoring the editor session.";
        return false;
    }

    if (!mContext.game->RestoreDebugEditorStage(
            sessionState.stageNumber,
            sessionState.stageYamlPath)) {
        outErrorMessage = "Failed to restore the edited stage: " +
                          sessionState.stageYamlPath;
        return false;
    }

    ApplyEditorSessionState(sessionState);
    return true;
}

void DebugUIRenderer::SetBuildRestartStatus(
    const std::string& message,
    bool isError)
{
    mBuildRestartStatus = message;
    mIsBuildRestartStatusError = isError;
}

EditorSessionState DebugUIRenderer::CaptureEditorSessionState() const
{
    EditorSessionState sessionState;
    if (!mContext.game) {
        return sessionState;
    }

    sessionState.stageNumber = mContext.game->GetCurrentStageNum();
    sessionState.stageYamlPath = mContext.game->GetCurrentStageYamlPath();
    sessionState.activeSectionIndex = static_cast<int>(mActiveSection);
    sessionState.sequenceEditorMenuIndex = mSelectedSequenceEditorMenu;
    sessionState.stageEditorMenuIndex = mStageEditorPanel.GetSelectedMenu();
    sessionState.isEditorShowing = mContext.game->GetIsDebugEditorShowing();
    sessionState.isSceneView = mContext.game->GetIsFreeCameraMode();
    sessionState.rightPanelWidth = mContext.layout.rightPanelWidth;
    sessionState.assetBrowserHeight = mContext.layout.assetBrowserHeight;

    if (CameraSystem* cameraSystem = mContext.game->GetCameraSystem()) {
        sessionState.sceneCameraPose = cameraSystem->GetDebugCameraPose();
    }

    if (Player* player = mContext.game->GetMainPlayer()) {
        sessionState.hasPlayerDebugPose = true;
        sessionState.playerPosition = player->GetPos();
        sessionState.playerUp = player->GetUpVec();
        sessionState.playerOrientation = player->GetOrientation();
        sessionState.playerPlanetIndex = player->GetCurrentPlanetNum();
    }

    const std::unordered_set<std::string>& selectedKeys =
        mSelectionController.GetSelectedKeys();
    sessionState.selectedActorKeys.assign(
        selectedKeys.begin(),
        selectedKeys.end());
    return sessionState;
}

void DebugUIRenderer::ApplyEditorSessionState(
    const EditorSessionState& sessionState)
{
    if (!mContext.game) {
        return;
    }

    constexpr int firstSectionIndex = static_cast<int>(EditorSection::BasicInfo);
    constexpr int lastSectionIndex = static_cast<int>(EditorSection::UserInterface);
    const int activeSectionIndex = std::clamp(
        sessionState.activeSectionIndex,
        firstSectionIndex,
        lastSectionIndex);
    mActiveSection = static_cast<EditorSection>(activeSectionIndex);
    mShouldSelectRestoredSection = true;

    mSelectedSequenceEditorMenu = std::clamp(
        sessionState.sequenceEditorMenuIndex,
        0,
        1);
    mStageEditorPanel.SetSelectedMenu(sessionState.stageEditorMenuIndex);
    mContext.layout.rightPanelWidth = sessionState.rightPanelWidth;
    mContext.layout.assetBrowserHeight = sessionState.assetBrowserHeight;

    mContext.game->SetDebugEditorShowing(sessionState.isEditorShowing);
    mContext.game->SetFreeCameraMode(sessionState.isSceneView);
    if (CameraSystem* cameraSystem = mContext.game->GetCameraSystem()) {
        cameraSystem->SetDebugCameraPose(sessionState.sceneCameraPose);
    }

    if (sessionState.hasPlayerDebugPose) {
        Player* player = mContext.game->GetMainPlayer();
        Stage* currentStage = mContext.game->GetCurrentStage();
        if (player && currentStage) {
            const std::vector<Planet*>& planets = currentStage->GetPlanets();
            const int planetIndex = sessionState.playerPlanetIndex;
            if (planetIndex >= 0 && planetIndex < static_cast<int>(planets.size())) {
                player->SetCurrentPlanet(planets[planetIndex]);
                player->SetCurrentPlanetNum(planetIndex);
            }
            player->SetPos(sessionState.playerPosition);
            player->SetUpVec(sessionState.playerUp);
            player->SetOrientation(sessionState.playerOrientation);
            player->SetVelocity(glm::vec3(0.0f));
        }
    }

    std::unordered_set<std::string> selectedKeys(
        sessionState.selectedActorKeys.begin(),
        sessionState.selectedActorKeys.end());
    if (selectedKeys.empty()) {
        mSelectionController.Clear();
        return;
    }

    if (selectedKeys.size() != 1) {
        mSelectionController.SetSelectedKeys(selectedKeys);
        mSelectionController.ConsumeRequestOpenPlacement();
        return;
    }

    Stage* currentStage = mContext.game->GetCurrentStage();
    const std::vector<StageActorInstance> actorInstances =
        StageActorQuery::CollectAllActorInstances(currentStage);
    for (const StageActorInstance& actorInstance : actorInstances) {
        if (!actorInstance.actor) {
            continue;
        }

        if (selectedKeys.contains(StageActorQuery::MakeKey(actorInstance.ref))) {
            mSelectionController.SetSingleSelection(
                actorInstance.actor,
                actorInstance.ref);
            mSelectionController.ConsumeRequestOpenPlacement();
            return;
        }
    }

    mSelectionController.Clear();
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
    const auto restoredTabFlags = [this](EditorSection section) {
        return mShouldSelectRestoredSection && mActiveSection == section
            ? ImGuiTabItemFlags_SetSelected
            : ImGuiTabItemFlags_None;
    };

    if (ImGui::BeginTabBar("DebugMainTabs")) {
        if (ImGui::BeginTabItem("基本情報", nullptr, restoredTabFlags(EditorSection::BasicInfo))) {
            activeSection = EditorSection::BasicInfo;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パラメータ調整", nullptr, restoredTabFlags(EditorSection::Parameters))) {
            activeSection = EditorSection::Parameters;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("パーティクル", nullptr, restoredTabFlags(EditorSection::Particles))) {
            activeSection = EditorSection::Particles;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("演出エディタ", nullptr, restoredTabFlags(EditorSection::Sequences))) {
            activeSection = EditorSection::Sequences;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("チュートリアル", nullptr, restoredTabFlags(EditorSection::Tutorials))) {
            activeSection = EditorSection::Tutorials;
            ImGui::EndTabItem();
        }

        const ImGuiTabItemFlags stageEditorTabFlags =
            requestedStageEditor
                ? ImGuiTabItemFlags_SetSelected
                : restoredTabFlags(EditorSection::Stage);

        if (ImGui::BeginTabItem("ステージエディタ", nullptr, stageEditorTabFlags)) {
            activeSection = EditorSection::Stage;
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("UI調整", nullptr, restoredTabFlags(EditorSection::UserInterface))) {
            activeSection = EditorSection::UserInterface;
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    mShouldSelectRestoredSection = false;

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
    DrawBuildRestartControls();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void DebugUIRenderer::AlignFreeCameraUpToSelectedActor()
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

void DebugUIRenderer::DrawBuildRestartControls()
{
    if (ImGui::Button("Build & Restart")) {
        std::string restartErrorMessage;
        if (!mContext.game ||
            !mContext.game->RequestEditorBuildAndRestart(restartErrorMessage)) {
            SetBuildRestartStatus(restartErrorMessage, true);
        } else {
            SetBuildRestartStatus("Building...", false);
        }
    }

    if (mBuildRestartStatus.empty()) {
        return;
    }

    ImGui::SameLine();
    const ImVec4 statusColor = mIsBuildRestartStatusError
        ? ImVec4(1.0f, 0.38f, 0.32f, 1.0f)
        : ImVec4(0.42f, 0.88f, 0.50f, 1.0f);
    ImGui::TextColored(statusColor, "%s", mBuildRestartStatus.c_str());
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
