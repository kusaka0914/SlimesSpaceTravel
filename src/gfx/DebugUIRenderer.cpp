#include "DebugUIRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/Platform.h"
#include "component/PlatformBehaviorComponents.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/session/EditorSessionState.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"


#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_set>

namespace {
void DrawUGCEraserActiveIndicator(bool isActive)
{
    if (!isActive) {
        return;
    }

    const ImVec2 min = ImGui::GetItemRectMin();
    const ImVec2 max = ImGui::GetItemRectMax();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(
        min,
        max,
        IM_COL32(255, 70, 45, 255),
        7.0f,
        0,
        3.0f);
    drawList->AddRectFilled(
        ImVec2(min.x + 4.0f, min.y + 4.0f),
        ImVec2(min.x + 27.0f, min.y + 19.0f),
        IM_COL32(205, 35, 25, 235),
        4.0f);
    drawList->AddText(
        ImVec2(min.x + 7.0f, min.y + 4.0f),
        IM_COL32(255, 255, 255, 255),
        "ON");
}

void DrawUGCSelectionActiveIndicator(bool isActive)
{
    if (!isActive) {
        return;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRect(
        ImGui::GetItemRectMin(),
        ImGui::GetItemRectMax(),
        IM_COL32(255, 201, 46, 255),
        10.0f,
        0,
        3.0f);
}

void DrawUGCWarningTriangle(
    ImDrawList* drawList,
    const ImVec2& center)
{
    if (!drawList) {
        return;
    }

    constexpr float halfWidth = 18.0f;
    constexpr float topOffset = 20.0f;
    constexpr float bottomOffset = 14.0f;
    const ImVec2 top(center.x, center.y - topOffset);
    const ImVec2 bottomLeft(
        center.x - halfWidth,
        center.y + bottomOffset);
    const ImVec2 bottomRight(
        center.x + halfWidth,
        center.y + bottomOffset);
    drawList->AddTriangleFilled(
        top,
        bottomLeft,
        bottomRight,
        IM_COL32(255, 213, 45, 245));
    drawList->AddTriangle(
        top,
        bottomLeft,
        bottomRight,
        IM_COL32(55, 42, 5, 255),
        2.5f);
    drawList->AddRectFilled(
        ImVec2(center.x - 2.5f, center.y - 9.0f),
        ImVec2(center.x + 2.5f, center.y + 3.0f),
        IM_COL32(55, 42, 5, 255),
        2.0f);
    drawList->AddCircleFilled(
        ImVec2(center.x, center.y + 8.0f),
        2.8f,
        IM_COL32(55, 42, 5, 255));
}
}

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer, &mAssetCatalog},
      mUGCWorkPanel(
          mContext,
          [this]() {
              mSelectionController.Clear();
              HandleUGCSelectionMode();
              mUGCConnectionSwitchRef.reset();
              mIsUGCSelectionDragging = false;
              mIsUGCMovingPlatformDestinationDrag = false;
              mEditCommandController.ClearHistory();
              mUGCEditLayer = 0;
              mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
              mSelectionController.SetUGCEditLayer(mUGCEditLayer);
              mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
              mContext.game->ReloadCurrentStage();
          }),
      mUGCModelThumbnailRenderer(
          std::make_unique<EditorModelThumbnailRenderer>(game)),
      mPerformancePanel(mContext),
      mCameraPanel(mContext),
      mUIPanel(mContext),
      mParameterPanel(mContext, mCameraPanel),
      mParticleEffectPanel(mContext),
      mSequencePanel(mContext),
      mEndingRollPanel(mContext),
      mStorybookPanel(mContext),
      mTutorialPanel(mContext),
      mAssetBrowserPanel(mContext),
      mStageAddActorPanel(mContext),
      mStagePlanetPanel(mContext),
      mStageActorYamlWriter(mContext),
      mSelectionController(mContext),
      mStagePlacementPanel(
          mContext,
          mSelectionController,
          mStageActorYamlWriter,
          [this]() { mEditCommandController.PushUndo(); }),
      mEditCommandController(mContext, mSelectionController),
      mStageDeleteActorPanel(mContext, mEditCommandController),
      mStageEditorPanel(
          mContext,
          mStageAddActorPanel,
          mStagePlanetPanel,
          mStagePlacementPanel,
          mStageDeleteActorPanel,
          mStageActorYamlWriter,
          mSelectionController,
          [this]() { return mEditCommandController.RestoreUndo(); },
          [this]() { return mEditCommandController.RestoreRedo(); }),
      mGizmoController(
          mContext, mSelectionController, [this]() { mEditCommandController.PushUndo(); },
          [this]() {
              mStagePlanetPanel.SaveEditorAuthoredTransforms();
          })
{
    mStagePlanetPanel.SetSaveDependentActorTransformsCallback(
        [this]() { mStageActorYamlWriter.SaveEditorAuthoredTransforms(); });
    mStageAddActorPanel.SetSelectionController(&mSelectionController);
    mStageAddActorPanel.SetPushUndoCallback(
        [this]() { mEditCommandController.PushUndo(); });
    mStageAddActorPanel.SetPlacementCompletedCallback([this]() {
        if (mActiveUGCPresetKind) {
            mUGCEditorTutorial.RecordPlacement(
                *mActiveUGCPresetKind,
                mUGCPlatformFootprintSideLength);
        }
    });
    mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
    mSelectionController.SetUGCEditLayer(mUGCEditLayer);
    if (mContext.game) {
        mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
    }
}

void DebugUIRenderer::HandleUGCUndo()
{
    const bool wasRestored = mEditCommandController.RestoreUndo();
    mUGCEditorTutorial.RecordUndo(wasRestored);
    mUGCStatus = wasRestored
        ? "1つ前の状態に戻しました"
        : "戻せる操作がありません";
}

void DebugUIRenderer::HandleUGCRedo()
{
    mUGCStatus = mEditCommandController.RestoreRedo()
        ? "戻した操作をやり直しました"
        : "やり直せる操作がありません";
}

void DebugUIRenderer::HandleUGCEraserToggle()
{
    if (!mIsUGCEraserMode) {
        mIsUGCEraserMode = true;
        mUGCPresetBeforeEraser = mStageAddActorPanel.IsPlacementActive()
            ? mActiveUGCPresetKind
            : std::nullopt;
        mStageAddActorPanel.CancelPlacement();
        mUGCStatus = "消したいものをクリックしてください";
        return;
    }

    mIsUGCEraserMode = false;
    if (mUGCPresetBeforeEraser &&
        mStageAddActorPanel.ActivateUGCPreset(*mUGCPresetBeforeEraser)) {
        mActiveUGCPresetKind = mUGCPresetBeforeEraser;
        mUGCStatus = "置く状態に戻りました";
    } else {
        mUGCStatus = "選択モードに戻りました";
    }
    mUGCPresetBeforeEraser.reset();
}

void DebugUIRenderer::HandleUGCSelectionMode()
{
    mIsUGCEraserMode = false;
    mUGCPresetBeforeEraser.reset();
    mActiveUGCPresetKind.reset();
    mStageAddActorPanel.CancelPlacement();
    mUGCStatus = "選びたいものをクリックしてください";
}

void DebugUIRenderer::OpenUGCEditorMenu()
{
    mShouldOpenUGCEditorMenu = true;
}

void DebugUIRenderer::HandleUGCZoom(float distanceMultiplier)
{
    AdjustUGCViewDistance(distanceMultiplier);
    mUGCEditorTutorial.RecordViewAdjustment();
}

void DebugUIRenderer::HandleUGCEditorTutorialReturnedFromPlaytest()
{
    mUGCEditorTutorial.RecordReturnedFromPlaytest();
}

void DebugUIRenderer::HandleUGCLayerChange(int layerDelta)
{
    const bool isMovingSelection =
        mIsUGCSelectionDragging &&
        mSelectionController.GetSelectedActorCount() > 0;
    const int previousLayer = mUGCEditLayer;
    ChangeUGCEditLayer(layerDelta);
    if (mUGCEditLayer != previousLayer) {
        mUGCEditorTutorial.RecordLayerChange(
            layerDelta,
            isMovingSelection);
    }
}

void DebugUIRenderer::HandleUGCSelectionGridMove(int gridX, int gridZ)
{
    if (mIsUGCEraserMode || mStageAddActorPanel.IsPlacementActive() ||
        mSelectionController.GetSelectedActorCount() == 0) {
        return;
    }
    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 movement(
        static_cast<float>(gridX) * gridSize, 0.0f,
        static_cast<float>(gridZ) * gridSize);
    mEditCommandController.PushUndo();
    std::vector<StageActorRef> selectedRefs;
    for (const StageActorInstance& selected :
         mSelectionController.CollectSelectedActorInstances()) {
        selectedRefs.push_back(selected.ref);
    }
    if (mSelectionController.IsMovingPlatformDestinationSelected()) {
        const bool destinationMoved =
            mStageAddActorPanel.TryTranslateUGCMovingPlatformDestinations(
                selectedRefs, movement);
        if (destinationMoved) {
            mSelectionController.Clear();
            mUGCStatus = "移動先を1マス動かしました";
        } else {
            mUGCStatus = "移動先を動かせませんでした";
        }
        return;
    }

    mSelectionController.MoveSelectedActorsByDelta(movement);
    const bool updatedUGCPlatform =
        mStageAddActorPanel.TryTranslateUGCPlatformCells(
            selectedRefs, movement);
    if (updatedUGCPlatform) {
        mSelectionController.Clear();
    }
    mUGCStatus = "選んだものを1マス動かしました";
}

bool DebugUIRenderer::SaveEditorSession(
    const std::string& filePath,
    std::string& outErrorMessage)
{


    // 実行時の移動は保存せず、編集操作で変更したTransformだけを保存する。
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
        4);
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

    if (mContext.game && mContext.game->GetIsUGCMode()) {
        RegisterUGCUIEditorElements();
    }

    if (activeSection == EditorSection::Stage) {
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

    if (mContext.game && mContext.game->GetIsUGCMode()) {
        DrawUGCDebugEditorOverlay();
    }
}

void DebugUIRenderer::RegisterUGCUIEditorElements()
{
    if (!mContext.uiRenderer) {
        return;
    }

    UILoadSystem* uiLoadSystem =
        mContext.uiRenderer->GetUILoadSystem();
    if (!uiLoadSystem) {
        return;
    }




    const auto findElement = [&](const std::string& id)
        -> const UILoadSystem::CustomElement* {
        const auto& elements = uiLoadSystem->GetCustomElements();
        const auto it = std::find_if(
            elements.begin(), elements.end(),
            [&](const UILoadSystem::CustomElement& element) {
                return element.screen == "ugc" && element.id == id;
            });
        return it == elements.end() ? nullptr : &*it;
    };
    const auto ensureButtonElement =
        [&](const char* id,
            const char* displayName,
            float xRatio,
            float yRatio) {
            if (findElement(id)) {
                return;
            }

            const std::size_t index = uiLoadSystem->AddCustomElement(
                UILoadSystem::CustomElementType::Panel, "ugc", id);
            UILoadSystem::CustomElement& element =
                uiLoadSystem->GetCustomElements()[index];
            element.displayName = displayName;
            element.visibleByDefault = false;
            element.xRatio = xRatio;
            element.yRatio = yRatio;
            element.widthRatio = 0.026f;
            element.heightRatio = 0.026f;
        };

    const UILoadSystem::CustomElement* presetTools =
        findElement("presetTools");
    const float presetX = presetTools ? presetTools->xRatio : 0.40625f;
    const float presetY = presetTools ? presetTools->yRatio : 0.0f;
    ensureButtonElement("presetPlatform", "足場", presetX, presetY);
    ensureButtonElement("presetEnemy", "敵", presetX + 0.031f, presetY);
    ensureButtonElement("presetPlanet", "惑星", presetX + 0.062f, presetY);
    ensureButtonElement("presetSwitch", "スイッチ", presetX + 0.093f, presetY);
    ensureButtonElement("presetGoal", "ゴール", presetX + 0.124f, presetY);
    ensureButtonElement("presetMoving", "移動足場", presetX + 0.155f, presetY);
    ensureButtonElement("presetFading", "消える足場", presetX + 0.186f, presetY);
    ensureButtonElement("presetAdhesive", "くっつき足場", presetX + 0.217f, presetY);
    ensureButtonElement("presetTwoPlayer", "2人用スイッチ", presetX + 0.248f, presetY);

    const UILoadSystem::CustomElement* quickTools =
        findElement("quickTools");
    const float quickX = quickTools ? quickTools->xRatio : 0.951f;
    const float quickY = quickTools ? quickTools->yRatio : 0.087f;
    ensureButtonElement("eraser", "消しゴム", quickX, quickY);
    ensureButtonElement("undo", "1つ戻す", quickX, quickY + 0.030f);
    ensureButtonElement("redo", "やり直す", quickX, quickY + 0.060f);

    const UILoadSystem::CustomElement* keyboardTools =
        findElement("keyboardTools");
    const float keyboardX = keyboardTools ? keyboardTools->xRatio : 0.00625f;
    const float keyboardY = keyboardTools ? keyboardTools->yRatio : 0.057f;
    ensureButtonElement("layerUp", "上のだん", keyboardX, keyboardY);
    ensureButtonElement("layerDown", "下のだん", keyboardX, keyboardY + 0.030f);
    ensureButtonElement("zoomIn", "近づく", keyboardX, keyboardY + 0.060f);
    ensureButtonElement("zoomOut", "遠ざかる", keyboardX, keyboardY + 0.090f);
    ensureButtonElement("previewView", "下から見る", keyboardX, keyboardY + 0.120f);

    const auto isLegacyToolbarPanel = [](const std::string& id) {
        return id == "presetTools" || id == "quickTools" ||
               id == "keyboardTools";
    };
    std::vector<const UILoadSystem::CustomElement*> editableElements;
    for (const UILoadSystem::CustomElement& element :
         uiLoadSystem->GetCustomElements()) {
        if (element.screen == "ugc" && !isLegacyToolbarPanel(element.id)) {
            editableElements.push_back(&element);
        }
    }
    std::stable_sort(
        editableElements.begin(), editableElements.end(),
        [](const UILoadSystem::CustomElement* left,
           const UILoadSystem::CustomElement* right) {
            return left->zOrder < right->zOrder;
        });
    for (const UILoadSystem::CustomElement* element : editableElements) {
        mContext.uiRenderer->RecordCustomUIElementForEditor(*element);
    }
}

void DebugUIRenderer::DrawUGCDebugEditorOverlay()
{
    if (!mContext.gameViewport.IsValid()) {
        return;
    }

    const ImVec2 viewportMin(
        mContext.gameViewport.x,
        mContext.gameViewport.y);
    const ImVec2 viewportMax(
        mContext.gameViewport.x + mContext.gameViewport.width,
        mContext.gameViewport.y + mContext.gameViewport.height);
    const ImVec2 viewportSize(
        viewportMax.x - viewportMin.x,
        viewportMax.y - viewportMin.y);

    const bool isAdjustingUGCUI =
        mActiveSection == EditorSection::UserInterface;
    const ImGuiWindowFlags overlayFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize |
        (isAdjustingUGCUI ? ImGuiWindowFlags_NoInputs : 0);

    struct UGCControlLayout {
        ImVec2 position;
        ImVec2 size;
    };
    const auto resolveUGCControlLayout =
        [&](const char* id,
            const ImVec2& fallbackPosition,
            const ImVec2& fallbackSize = ImVec2(52.0f, 52.0f)) {
            if (!mContext.uiRenderer ||
                !mContext.uiRenderer->GetUILoadSystem()) {
                return UGCControlLayout{fallbackPosition, fallbackSize};
            }
            for (const UILoadSystem::CustomElement& element :
                 mContext.uiRenderer->GetUILoadSystem()->
                     GetCustomElements()) {
                if (element.screen == "ugc" && element.id == id) {
                    return UGCControlLayout{
                        ImVec2(
                            viewportMin.x +
                                viewportSize.x * element.xRatio,
                            viewportMin.y +
                                viewportSize.x * element.yRatio),
                        ImVec2(
                            std::max(1.0f,
                                viewportSize.x * element.widthRatio),
                            std::max(1.0f,
                                viewportSize.x * element.heightRatio))};
                }
            }
            return UGCControlLayout{fallbackPosition, fallbackSize};
        };
    const auto getUGCControlZOrder = [&](const char* id) {
        if (!mContext.uiRenderer || !mContext.uiRenderer->GetUILoadSystem()) {
            return 0;
        }
        for (const UILoadSystem::CustomElement& element :
             mContext.uiRenderer->GetUILoadSystem()->GetCustomElements()) {
            if (element.screen == "ugc" && element.id == id) {
                return element.zOrder;
            }
        }
        return 0;
    };

    const auto drawActionIcon = [&]
        (const char* id,
         const char* texturePath,
         const char* tooltip,
         const ImVec2& size = ImVec2(52.0f, 52.0f)) {
        ImGui::PushID(id);
        const bool hasTexture =
            mContext.uiRenderer &&
            mContext.uiRenderer->RegisterCustomUITexture(texturePath);
        const GLuint texture = hasTexture
            ? mContext.uiRenderer->GetCustomUITextureHandle(texturePath)
            : 0;
        ImGui::PushStyleVar(
            ImGuiStyleVar_FramePadding,
            ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(1, 1, 1, 0.18f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(1, 1, 1, 0.32f));
        const bool clicked = texture != 0
            ? ImGui::ImageButton(
                  "##ugcDebugActionIcon",
                  static_cast<ImTextureID>(texture),
                  size,
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(tooltip, size);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        DrawUGCEraserActiveIndicator(
            std::strcmp(id, "eraser") == 0 && mIsUGCEraserMode);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
        ImGui::PopID();
        return clicked;
    };

    struct PresetButton {
        const char* label;
        UGCPresetKind kind;
    };
    constexpr std::array<PresetButton, 9> presetButtons = {{
        {"惑星", UGCPresetKind::EllipsePlanet},
        {"足場", UGCPresetKind::NormalPlatform},
        {"移動足場", UGCPresetKind::MovingPlatform},
        {"消える足場", UGCPresetKind::FadingPlatform},
        {"くっつき足場", UGCPresetKind::AdhesivePlatform},
        {"スイッチ", UGCPresetKind::PressureSwitch},
        {"2人用スイッチ", UGCPresetKind::TwoPlayerSwitch},
        {"敵", UGCPresetKind::NormalEnemy},
        {"ゴール", UGCPresetKind::GoalStar},
    }};
    if (mUGCModelThumbnailRenderer) {
        mUGCModelThumbnailRenderer->BeginFrame();
    }
    constexpr float presetIconSize = 52.0f;
    constexpr float presetSlotWidth = 62.0f;
    constexpr std::array<const char*, 9> presetIds = {{
        "presetPlanet", "presetPlatform", "presetMoving", "presetFading",
        "presetAdhesive", "presetSwitch", "presetTwoPlayer", "presetEnemy",
        "presetGoal",
    }};
    std::array<std::size_t, 9> presetDrawOrder = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    std::stable_sort(
        presetDrawOrder.begin(), presetDrawOrder.end(),
        [&](std::size_t left, std::size_t right) {
            return getUGCControlZOrder(presetIds[left]) <
                   getUGCControlZOrder(presetIds[right]);
        });
    for (const std::size_t presetIndex : presetDrawOrder) {
        const PresetButton& preset = presetButtons[presetIndex];
        const UGCControlLayout layout = resolveUGCControlLayout(
            presetIds[presetIndex],
            ImVec2(
                viewportMin.x +
                    std::max(
                        6.0f,
                        (viewportSize.x -
                         presetSlotWidth * presetButtons.size()) * 0.5f) +
                    presetSlotWidth * static_cast<float>(presetIndex),
                viewportMin.y + 6.0f),
            ImVec2(presetIconSize, presetIconSize));
        const std::string windowId =
            std::string("###UGCDebugPreset_") + presetIds[presetIndex];
        ImGui::SetNextWindowPos(layout.position, ImGuiCond_Always);
        ImGui::Begin(windowId.c_str(), nullptr, overlayFlags);
        ImGui::PushID(preset.label);
        const UGCPresetVisual& presetVisual =
            GetUGCPresetVisual(preset.kind);
        const GLuint thumbnail = mUGCModelThumbnailRenderer
            ? mUGCModelThumbnailRenderer->ResolveThumbnail(
                  presetVisual.modelPath,
                  presetVisual.thumbnailScale,
                  presetVisual.initialTextureOverridePath)
            : 0;
        const bool isActive = mActiveUGCPresetKind == preset.kind;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 9.0f);
        ImGui::PushStyleVar(
            ImGuiStyleVar_FrameBorderSize,
            isActive ? 3.0f : 1.0f);
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            isActive
                ? ImVec4(1.0f, 0.79f, 0.18f, 1.0f)
                : ImVec4(0.96f, 0.98f, 1.0f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(1.0f, 0.88f, 0.40f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(1.0f, 0.72f, 0.08f, 1.0f));
        const bool clicked = thumbnail != 0
            ? ImGui::ImageButton(
                  "##ugcDebugPresetIcon",
                  static_cast<ImTextureID>(thumbnail),
                  layout.size,
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(
                preset.label,
                layout.size);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", preset.label);
        }
        if (clicked) {
            mIsUGCEraserMode = false;
            if (mStageAddActorPanel.ActivateUGCPreset(preset.kind)) {
                mActiveUGCPresetKind = preset.kind;
                mUGCStatus = std::string(preset.label) + "を選びました";
            } else {
                mUGCStatus = std::string(preset.label) +
                    "を選べませんでした";
            }
        }
        ImGui::PopID();
        ImGui::End();
    }

    const UGCControlLayout menuLayout = resolveUGCControlLayout(
        "menu", ImVec2(viewportMax.x - 60.0f, viewportMin.y + 6.0f));
    ImGui::SetNextWindowPos(
        menuLayout.position,
        ImGuiCond_Always);
    ImGui::Begin("###UGCDebugMenu", nullptr, overlayFlags);
    if (drawActionIcon(
            "menu",
            "textures/ugc_ui/editor_action_menu.png",
            "メニュー",
            menuLayout.size)) {
        ImGui::OpenPopup("メニュー###UGCDebugProductMenu");
    }
    if (ImGui::BeginPopup("メニュー###UGCDebugProductMenu")) {
        if (ImGui::MenuItem("保存・開く")) {
            mShouldOpenUGCWorkManagement = true;
        }
        if (ImGui::MenuItem("完成チェック")) {
            StartUGCVerification();
        }
        if (ImGui::MenuItem("タイトルへ戻る")) {
            mContext.game->ExitUGCMode();
        }
        ImGui::EndPopup();
    }
    ImGui::End();

    const auto drawUGCActionControl =
        [&](const char* id,
            const char* texturePath,
            const char* tooltip,
            const ImVec2& fallbackPosition) {
            const UGCControlLayout layout = resolveUGCControlLayout(
                id, fallbackPosition);
            const std::string windowId =
                std::string("###UGCDebugAction_") + id;
            ImGui::SetNextWindowPos(layout.position, ImGuiCond_Always);
            ImGui::Begin(windowId.c_str(), nullptr, overlayFlags);
            const bool clicked =
                drawActionIcon(id, texturePath, tooltip, layout.size);
            ImGui::End();
            return clicked;
        };



    struct ActionControl {
        const char* id;
        const char* texturePath;
        const char* tooltip;
        ImVec2 fallbackPosition;
    };
    std::vector<ActionControl> actionControls = {
        {"eraser", "textures/ugc_ui/editor_action_eraser.png",
         mIsUGCEraserMode ? "消しゴム：ON" : "消しゴム",
         ImVec2(viewportMax.x - 60.0f, viewportMin.y + 64.0f)},
        {"undo", "textures/ugc_ui/editor_action_undo.png", "1つ戻す",
         ImVec2(viewportMax.x - 60.0f, viewportMin.y + 122.0f)},
        {"redo", "textures/ugc_ui/editor_action_redo.png", "やり直す",
         ImVec2(viewportMax.x - 60.0f, viewportMin.y + 180.0f)},
        {"layerUp", "textures/ugc_ui/editor_action_layer_up.png", "上のだん",
         ImVec2(viewportMin.x + 6.0f, viewportMin.y + 68.0f)},
        {"layerDown", "textures/ugc_ui/editor_action_layer_down.png", "下のだん",
         ImVec2(viewportMin.x + 6.0f, viewportMin.y + 126.0f)},
        {"zoomIn", "textures/ugc_ui/editor_action_zoom_in.png", "近づく",
         ImVec2(viewportMin.x + 6.0f, viewportMin.y + 184.0f)},
        {"zoomOut", "textures/ugc_ui/editor_action_zoom_out.png", "遠ざかる",
         ImVec2(viewportMin.x + 6.0f, viewportMin.y + 242.0f)},
        {"previewView", "textures/ugc_ui/editor_action_preview_view.png",
         mContext.game->GetIsUGCPreviewViewedFromBelow()
             ? "上から見る" : "下から見る",
         ImVec2(viewportMin.x + 6.0f, viewportMin.y + 300.0f)},
    };
    std::stable_sort(
        actionControls.begin(), actionControls.end(),
        [&](const ActionControl& left, const ActionControl& right) {
            return getUGCControlZOrder(left.id) <
                   getUGCControlZOrder(right.id);
        });
    for (const ActionControl& control : actionControls) {
        if (!drawUGCActionControl(
                control.id, control.texturePath, control.tooltip,
                control.fallbackPosition)) {
            continue;
        }
        if (std::strcmp(control.id, "eraser") == 0) {
            HandleUGCEraserToggle();
        } else if (std::strcmp(control.id, "undo") == 0) {
            HandleUGCUndo();
        } else if (std::strcmp(control.id, "redo") == 0) {
            HandleUGCRedo();
        } else if (std::strcmp(control.id, "layerUp") == 0) {
            HandleUGCLayerChange(1);
        } else if (std::strcmp(control.id, "layerDown") == 0) {
            HandleUGCLayerChange(-1);
        } else if (std::strcmp(control.id, "zoomIn") == 0) {
            HandleUGCZoom(0.85f);
        } else if (std::strcmp(control.id, "zoomOut") == 0) {
            HandleUGCZoom(1.18f);
        } else if (std::strcmp(control.id, "previewView") == 0) {
            ToggleUGCVerticalView();
        }
    }

    const UGCControlLayout playLayout = resolveUGCControlLayout(
        "play", ImVec2(viewportMin.x + 6.0f, viewportMax.y - 60.0f));
    ImGui::SetNextWindowPos(
        playLayout.position,
        ImGuiCond_Always);
    ImGui::Begin("###UGCDebugPlay", nullptr, overlayFlags);
    if (drawActionIcon(
            "play",
            "textures/ugc_ui/editor_action_play.png",
            "遊ぶ",
            playLayout.size)) {
        mContext.game->StartUGCPlaytest();
        ImGui::End();
        return;
    }
    ImGui::End();

    if (!IsUGCWorkManagementOpen()) {
        DrawUGCGridOverlay();
        DrawUGCStackBadges();
        DrawUGCPlacementPreview();
        DrawUGCPreviewOverlay();
    }
    DrawUGCWorkManagement();
}

bool DebugUIRenderer::IsUGCWorkManagementOpen() const
{
    return mShouldOpenUGCWorkManagement ||
        ImGui::IsPopupOpen(
            "作品管理###UGCWorkManagement",
            ImGuiPopupFlags_AnyPopupLevel);
}

void DebugUIRenderer::DrawUGCWorkManagement()
{
    if (mShouldOpenUGCWorkManagement) {
        ImGui::OpenPopup("作品管理###UGCWorkManagement");
        mShouldOpenUGCWorkManagement = false;
    }
    mUGCWorkPanel.DrawManagement(mUGCStatus);
}

void DebugUIRenderer::DrawUGCTutorialHighlightForLastItem(
    bool shouldHighlight) const
{
    if (!shouldHighlight) {
        return;
    }
    const float pulse =
        0.5f + 0.5f * std::sin(static_cast<float>(ImGui::GetTime()) * 5.0f);
    const int alpha = static_cast<int>(190.0f + pulse * 65.0f);
    ImGui::GetWindowDrawList()->AddRect(
        ImGui::GetItemRectMin(),
        ImGui::GetItemRectMax(),
        IM_COL32(255, 215, 45, alpha),
        10.0f,
        0,
        5.0f);
}

void DebugUIRenderer::DrawUGCEditorTutorial()
{
    if (!mContext.game) {
        return;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    constexpr ImGuiWindowFlags tutorialWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;
    if (!mUGCEditorTutorial.IsActive()) {
        ImGui::SetNextWindowPos(
            ImVec2(
                viewport->WorkPos.x + viewport->WorkSize.x - 230.0f,
                viewport->WorkPos.y + 16.0f),
            ImGuiCond_Always);
        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
        ImGui::Begin(
            "###UGCTutorialReplay",
            nullptr,
            tutorialWindowFlags |
                ImGuiWindowFlags_NoBackground);
        if (ImGui::Button("操作練習", ImVec2(154.0f, 46.0f))) {
            mContext.game->StartUGCEditorTutorial();
        }
        ImGui::End();
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(3);
        return;
    }

    const UGCEditorTutorialStep step = mUGCEditorTutorial.GetStep();
    const bool isWelcome = step == UGCEditorTutorialStep::Welcome;
    const bool isComplete = step == UGCEditorTutorialStep::Complete;
    const ImVec2 panelSize(
        std::min(680.0f, viewport->WorkSize.x - 48.0f),
        isWelcome || isComplete ? 246.0f : 192.0f);
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
            viewport->WorkPos.y + 112.0f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.0f));
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(24.0f, 20.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 12.0f));
    ImGui::PushStyleColor(
        ImGuiCol_WindowBg,
        ImVec4(0.025f, 0.055f, 0.10f, 0.98f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.25f, 0.66f, 1.0f, 0.90f));
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_PlotHistogram,
        ImVec4(0.18f, 0.67f, 1.0f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_FrameBg,
        ImVec4(0.07f, 0.14f, 0.23f, 1.0f));
    ImGui::Begin(
        "###UGCEditorTutorial",
        nullptr,
        tutorialWindowFlags);

    ImGui::SetWindowFontScale(1.22f);
    ImGui::TextUnformatted("操作練習");
    ImGui::SetWindowFontScale(1.0f);
    if (!isComplete) {
        const char* skipLabel = "練習をスキップ";
        const float skipButtonWidth = 138.0f;
        ImGui::SameLine(
            ImGui::GetWindowContentRegionMax().x - skipButtonWidth);
        if (ImGui::Button(skipLabel, ImVec2(skipButtonWidth, 34.0f))) {
            mUGCEditorTutorial.Stop();
            mContext.game->FinishUGCEditorTutorial(false);
            ImGui::End();
            ImGui::PopStyleColor(7);
            ImGui::PopStyleVar(5);
            return;
        }
    }

    if (!isWelcome) {
        const std::string actionCountText =
            isComplete
            ? "完了"
            : std::to_string(mUGCEditorTutorial.GetCurrentActionNumber()) +
                " / " + std::to_string(mUGCEditorTutorial.GetActionCount());
        ImGui::TextColored(
            ImVec4(1.0f, 0.84f, 0.25f, 1.0f),
            "%s",
            actionCountText.c_str());
        ImGui::SameLine();
        ImGui::ProgressBar(
            mUGCEditorTutorial.GetProgressRatio(),
            ImVec2(-1.0f, 12.0f),
            "");
    }

    ImGui::Separator();
    const std::string instruction =
        mUGCEditorTutorial.GetInstruction();
    ImGui::SetWindowFontScale(1.14f);
    ImGui::TextWrapped("%s", instruction.c_str());
    ImGui::SetWindowFontScale(1.0f);

    if (isWelcome) {
        ImGui::SetCursorPosY(panelSize.y - 66.0f);
        if (ImGui::Button("練習をはじめる", ImVec2(220.0f, 44.0f))) {
            mUGCEditorTutorial.AdvanceFromWelcome();
        }
    } else if (isComplete) {
        ImGui::SetCursorPosY(panelSize.y - 66.0f);
        if (ImGui::Button(
                "通常のステージ作成へ",
                ImVec2(240.0f, 44.0f))) {
            mUGCEditorTutorial.Stop();
            mContext.game->FinishUGCEditorTutorial(true);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar(5);
}

void DebugUIRenderer::StartUGCVerification()
{
    mUGCWorkPanel.StartVerification(mUGCStatus);
}

void DebugUIRenderer::DrawUGCWorkBrowser()
{
    mUGCWorkPanel.DrawBrowser();
}

bool DebugUIRenderer::CompleteUGCVerification(
    const std::string& workFileName)
{
    return mUGCWorkPanel.CompleteVerification(workFileName);
}

void DebugUIRenderer::DrawUGCEditor(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const bool isTutorialStage =
        mContext.game &&
        mContext.game->GetIsUGCEditorTutorialActive();
    if (isTutorialStage && !mUGCEditorTutorial.IsActive()) {
        mUGCEditorTutorial.Start();
    } else if (!isTutorialStage && mUGCEditorTutorial.IsActive()) {
        mUGCEditorTutorial.Stop();
    }

    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    constexpr float topBarHeight = 104.0f;
    const ImVec2 gameViewportMin(
        mainViewport->WorkPos.x,
        mainViewport->WorkPos.y);
    const ImVec2 gameViewportMax(
        mainViewport->WorkPos.x + mainViewport->WorkSize.x,
        mainViewport->WorkPos.y + mainViewport->WorkSize.y);
    struct UGCControlLayout {
        ImVec2 position;
        ImVec2 size;
    };
    const auto resolveUGCControlLayout =
        [&](const char* id,
            const ImVec2& fallbackPosition,
            const ImVec2& fallbackSize = ImVec2(64.0f, 64.0f)) {
        if (!mContext.uiRenderer || !mContext.uiRenderer->GetUILoadSystem()) {
            return UGCControlLayout{fallbackPosition, fallbackSize};
        }
        for (const UILoadSystem::CustomElement& element :
             mContext.uiRenderer->GetUILoadSystem()->GetCustomElements()) {
            if (element.screen == "ugc" && element.id == id) {
                return UGCControlLayout{
                    ImVec2(
                        mainViewport->WorkPos.x +
                            mainViewport->WorkSize.x * element.xRatio,
                        mainViewport->WorkPos.y +
                            mainViewport->WorkSize.x * element.yRatio),
                    ImVec2(
                        std::max(
                            1.0f,
                            mainViewport->WorkSize.x * element.widthRatio),
                        std::max(
                            1.0f,
                            mainViewport->WorkSize.x * element.heightRatio))};
            }
        }
        return UGCControlLayout{fallbackPosition, fallbackSize};
    };
    if (mUGCModelThumbnailRenderer) {
        mUGCModelThumbnailRenderer->BeginFrame();
    }

    constexpr ImGuiWindowFlags fixedWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    struct PresetButton {
        const char* id;
        const char* label;
        UGCPresetKind kind;
    };
    constexpr std::array<PresetButton, 9> presetButtons = {{
        {"presetPlanet", "惑星", UGCPresetKind::EllipsePlanet},
        {"presetPlatform", "足場", UGCPresetKind::NormalPlatform},
        {"presetMoving", "移動足場", UGCPresetKind::MovingPlatform},
        {"presetFading", "消える足場", UGCPresetKind::FadingPlatform},
        {"presetAdhesive", "くっつき足場", UGCPresetKind::AdhesivePlatform},
        {"presetSwitch", "スイッチ", UGCPresetKind::PressureSwitch},
        {"presetTwoPlayer", "2人用スイッチ", UGCPresetKind::TwoPlayerSwitch},
        {"presetEnemy", "敵", UGCPresetKind::NormalEnemy},
        {"presetGoal", "ゴール", UGCPresetKind::GoalStar},
    }};

    const bool usesUGCPlatformFootprint =
        mActiveUGCPresetKind == UGCPresetKind::NormalPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::MovingPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::FadingPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::AdhesivePlatform;
    const UGCControlLayout presetToolbarAnchor = resolveUGCControlLayout(
        "presetTools",
        mainViewport->WorkPos,
        ImVec2(720.0f, topBarHeight));
    if (usesUGCPlatformFootprint) {
        constexpr float presetToolbarWidthPixels = 720.0f;
        constexpr float presetOptionsWidthPixels = 260.0f;
        constexpr float presetOptionsHeightPixels = 40.0f;
        constexpr float presetOptionsTopOffsetPixels = 72.0f;
        const float presetOptionsLeftOffsetPixels =
            (presetToolbarWidthPixels - presetOptionsWidthPixels) * 0.5f;
        ImGui::SetNextWindowPos(
            ImVec2(
                presetToolbarAnchor.position.x +
                    presetOptionsLeftOffsetPixels,
                presetToolbarAnchor.position.y +
                    presetOptionsTopOffsetPixels),
            ImGuiCond_Always);
        ImGui::SetNextWindowSize(
            ImVec2(
                presetOptionsWidthPixels,
                presetOptionsHeightPixels),
            ImGuiCond_Always);
        ImGui::Begin(
            "###UGCPlatformFootprintOptions",
            nullptr,
            fixedWindowFlags);
        ImGui::TextUnformatted("大きさ");
        ImGui::SameLine();
        constexpr std::array<int, 3> footprintSideLengths = {1, 2, 3};
        constexpr std::array<const char*, 3> footprintLabels = {
            "1マス",
            "4マス",
            "9マス",
        };
        for (std::size_t index = 0;
             index < footprintSideLengths.size();
             ++index) {
            const bool isSelected =
                mUGCPlatformFootprintSideLength ==
                footprintSideLengths[index];
            if (ImGui::Selectable(
                    footprintLabels[index],
                    isSelected,
                    0,
                    ImVec2(52.0f, 0.0f))) {
                mUGCPlatformFootprintSideLength =
                    footprintSideLengths[index];
                mStageAddActorPanel.SetUGCPlatformFootprintSideLength(
                    footprintSideLengths[index]);
            }
            DrawUGCTutorialHighlightForLastItem(
                mUGCEditorTutorial.ShouldHighlightFootprintOptions() &&
                footprintSideLengths[index] > 1);
            ImGui::SameLine();
        }
        ImGui::End();
    }
    for (std::size_t presetIndex = 0;
         presetIndex < presetButtons.size();
         ++presetIndex) {
        const PresetButton& preset = presetButtons[presetIndex];
        const UGCControlLayout layout = resolveUGCControlLayout(
            preset.id,
            ImVec2(
                mainViewport->WorkPos.x +
                    mainViewport->WorkSize.x *
                        (0.40625f + 0.031f * static_cast<float>(presetIndex)),
                mainViewport->WorkPos.y),
            ImVec2(60.0f, 60.0f));
        const std::string windowId =
            std::string("###UGCPreset_") + preset.id;
        const ImVec2 framePadding = ImGui::GetStyle().FramePadding;
        const ImVec2 presetWindowSize(
            layout.size.x + framePadding.x * 2.0f,
            layout.size.y + framePadding.y * 2.0f);
        ImGui::SetNextWindowPos(layout.position, ImGuiCond_Always);
        ImGui::SetNextWindowSize(presetWindowSize, ImGuiCond_Always);
        ImGui::PushStyleVar(
            ImGuiStyleVar_WindowPadding,
            ImVec2(0.0f, 0.0f));
        ImGui::Begin(windowId.c_str(), nullptr, fixedWindowFlags);
        ImGui::PushID(preset.label);
        const UGCPresetVisual& presetVisual =
            GetUGCPresetVisual(preset.kind);
        const GLuint thumbnail = mUGCModelThumbnailRenderer
            ? mUGCModelThumbnailRenderer->ResolveThumbnail(
                  presetVisual.modelPath,
                  presetVisual.thumbnailScale,
                  presetVisual.initialTextureOverridePath)
            : 0;
        const bool isActivePreset = mActiveUGCPresetKind == preset.kind;
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, isActivePreset ? 4.0f : 2.0f);
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            isActivePreset ? ImVec4(1.0f, 0.79f, 0.18f, 1.0f)
                           : ImVec4(0.96f, 0.98f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.88f, 0.40f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.72f, 0.08f, 1.0f));
        const bool clicked = thumbnail != 0
            ? ImGui::ImageButton(
                  "##presetIcon",
                  static_cast<ImTextureID>(thumbnail),
                  layout.size,
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(preset.label, layout.size);
        DrawUGCTutorialHighlightForLastItem(
            mUGCEditorTutorial.IsActive() &&
            mUGCEditorTutorial.ShouldHighlightPreset(preset.kind));
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", preset.label);
        }
        if (clicked) {
            mIsUGCEraserMode = false;
            const bool activated = mStageAddActorPanel.ActivateUGCPreset(preset.kind);
            if (activated) mActiveUGCPresetKind = preset.kind;
            mUGCStatus = activated ? std::string(preset.label) + "を選びました"
                                     : std::string(preset.label) + "を選べませんでした";
        }
        ImGui::PopID();
        ImGui::End();
        ImGui::PopStyleVar();
    }

    constexpr ImGuiWindowFlags floatingButtonFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;
    const auto drawActionIcon = [&](const char* id,
                                    const char* texturePath,
                                    const char* tooltip,
                                    const ImVec2& size = ImVec2(64.0f, 64.0f),
                                    bool isSelected = false) {
        ImGui::PushID(id);
        const bool hasTexture = mContext.uiRenderer &&
            mContext.uiRenderer->RegisterCustomUITexture(texturePath);
        const GLuint texture = hasTexture
            ? mContext.uiRenderer->GetCustomUITextureHandle(texturePath)
            : 0;


        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.32f));
        const bool clicked = texture != 0
            ? ImGui::ImageButton(
                  "##actionIcon",
                  static_cast<ImTextureID>(texture),
                  size,
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(tooltip, size);
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        DrawUGCSelectionActiveIndicator(isSelected);
        DrawUGCEraserActiveIndicator(
            std::strcmp(id, "eraser") == 0 && mIsUGCEraserMode);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
        return clicked;
    };

    const UGCControlLayout presetReferenceLayout = resolveUGCControlLayout(
        "presetPlanet",
        ImVec2(
            mainViewport->WorkPos.x + mainViewport->WorkSize.x * 0.40625f,
            mainViewport->WorkPos.y),
        ImVec2(60.0f, 60.0f));
    const UGCControlLayout selectionPositionLayout = resolveUGCControlLayout(
        "selection",
        ImVec2(
            mainViewport->WorkPos.x + mainViewport->WorkSize.x * 0.375f,
            mainViewport->WorkPos.y),
        presetReferenceLayout.size);
    const UGCControlLayout selectionLayout{
        selectionPositionLayout.position,
        presetReferenceLayout.size};
    const bool isSelectionMode =
        !mIsUGCEraserMode && !mStageAddActorPanel.IsPlacementActive();
    const ImVec2 selectionWindowSize(
        selectionLayout.size.x + ImGui::GetStyle().FramePadding.x * 2.0f,
        selectionLayout.size.y + ImGui::GetStyle().FramePadding.y * 2.0f);
    ImGui::SetNextWindowPos(selectionLayout.position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(selectionWindowSize, ImGuiCond_Always);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0.0f, 0.0f));
    ImGui::Begin("###UGCSelection", nullptr, fixedWindowFlags);
    if (drawActionIcon(
            "selection",
            "textures/ugc_ui/editor_action_select.png",
            "選択モード",
            selectionLayout.size,
            isSelectionMode)) {
        HandleUGCSelectionMode();
    }
    DrawUGCTutorialHighlightForLastItem(
        mUGCEditorTutorial.ShouldHighlightSelection());
    ImGui::End();
    ImGui::PopStyleVar();

    const auto drawUGCActionControl =
        [&](const char* id,
            const char* texturePath,
            const char* tooltip,
            const ImVec2& fallbackPosition,
            bool shouldHighlight = false) {
            const UGCControlLayout layout = resolveUGCControlLayout(
                id,
                fallbackPosition);
            const std::string windowId =
                std::string("###UGCAction_") + id;
            ImGui::SetNextWindowPos(layout.position, ImGuiCond_Always);
            ImGui::SetNextWindowSize(layout.size, ImGuiCond_Always);
            ImGui::PushStyleVar(
                ImGuiStyleVar_WindowPadding,
                ImVec2(0.0f, 0.0f));
            ImGui::Begin(windowId.c_str(), nullptr, floatingButtonFlags);
            const bool clicked = drawActionIcon(id, texturePath, tooltip, layout.size);
            DrawUGCTutorialHighlightForLastItem(shouldHighlight);
            ImGui::End();
            ImGui::PopStyleVar();
            return clicked;
        };

    if (drawUGCActionControl(
            "eraser",
            "textures/ugc_ui/editor_action_eraser.png",
            mIsUGCEraserMode ? "消しゴム：ON" : "消しゴム",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 108.0f),
            mUGCEditorTutorial.ShouldHighlightEraser())) {
        HandleUGCEraserToggle();
    }
    if (drawUGCActionControl(
            "undo",
            "textures/ugc_ui/editor_action_undo.png",
            "1つ戻す",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 172.0f),
            mUGCEditorTutorial.ShouldHighlightUndo())) {
        HandleUGCUndo();
    }
    if (drawUGCActionControl(
            "redo",
            "textures/ugc_ui/editor_action_redo.png",
            "やり直す",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 236.0f))) {
        HandleUGCRedo();
    }
    if (drawUGCActionControl(
            "layerUp",
            "textures/ugc_ui/editor_action_layer_up.png",
            "上のだん",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 16.0f),
            mUGCEditorTutorial.ShouldHighlightLayerUp())) {
        HandleUGCLayerChange(1);
    }
    if (drawUGCActionControl(
            "layerDown",
            "textures/ugc_ui/editor_action_layer_down.png",
            "下のだん",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 80.0f),
            mUGCEditorTutorial.ShouldHighlightLayerDown())) {
        HandleUGCLayerChange(-1);
    }
    if (drawUGCActionControl(
            "zoomIn",
            "textures/ugc_ui/editor_action_zoom_in.png",
            "近づく",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 152.0f),
            mUGCEditorTutorial.ShouldHighlightZoom())) {
        HandleUGCZoom(0.85f);
    }
    if (drawUGCActionControl(
            "zoomOut",
            "textures/ugc_ui/editor_action_zoom_out_control.png",
            "遠ざかる",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 216.0f),
            mUGCEditorTutorial.ShouldHighlightZoom())) {
        HandleUGCZoom(1.18f);
    }
    if (drawUGCActionControl(
            "previewView",
            "textures/ugc_ui/editor_action_preview_view_control.png",
            mContext.game->GetIsUGCPreviewViewedFromBelow()
                ? "上から見る"
                : "下から見る",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 280.0f))) {
        ToggleUGCVerticalView();
    }

    const UGCControlLayout playLayout = resolveUGCControlLayout(
        "play",
        ImVec2(gameViewportMin.x + 16.0f, gameViewportMax.y - 74.0f));
    ImGui::SetNextWindowPos(playLayout.position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(playLayout.size, ImGuiCond_Always);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0.0f, 0.0f));
    ImGui::Begin("###UGCPlay", nullptr, floatingButtonFlags);
    if (drawActionIcon(
            "play",
            "textures/ugc_ui/editor_action_play.png",
            "遊ぶ",
            playLayout.size)) {
        mUGCEditorTutorial.RecordPlaytestStarted();
        mContext.game->StartUGCPlaytest();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    DrawUGCTutorialHighlightForLastItem(
        mUGCEditorTutorial.ShouldHighlightPlaytest());
    ImGui::End();
    ImGui::PopStyleVar();

    const UGCControlLayout menuLayout = resolveUGCControlLayout(
        "menu",
        ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 32.0f));
    ImGui::SetNextWindowPos(menuLayout.position, ImGuiCond_Always);
    ImGui::SetNextWindowSize(menuLayout.size, ImGuiCond_Always);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0.0f, 0.0f));
    ImGui::Begin("###UGCMenu", nullptr, floatingButtonFlags);
    const bool wasMenuButtonPressed = drawActionIcon(
        "menu",
        "textures/ugc_ui/editor_action_menu.png",
        "メニュー",
        menuLayout.size);
    const bool shouldToggleMenu =
        mShouldOpenUGCEditorMenu || wasMenuButtonPressed;
    mShouldOpenUGCEditorMenu = false;
    const bool wasMenuOpen = ImGui::IsPopupOpen(
        "メニュー###UGCProductMenu");
    if (shouldToggleMenu && !wasMenuOpen) {
        ImGui::OpenPopup("メニュー###UGCProductMenu");
    }

    constexpr float menuPanelWidth = 284.0f;
    constexpr float menuPanelHeight = 252.0f;
    ImGui::SetNextWindowPos(
        ImVec2(
            std::max(
                gameViewportMin.x + 16.0f,
                menuLayout.position.x - menuPanelWidth - 10.0f),
            menuLayout.position.y),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(menuPanelWidth, menuPanelHeight),
        ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 18.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 10.0f));
    ImGui::PushStyleColor(
        ImGuiCol_PopupBg,
        ImVec4(0.025f, 0.055f, 0.10f, 0.98f));
    ImGui::PushStyleColor(
        ImGuiCol_Border,
        ImVec4(0.25f, 0.66f, 1.0f, 0.90f));
    ImGui::PushStyleColor(
        ImGuiCol_Button,
        ImVec4(0.08f, 0.36f, 0.65f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonHovered,
        ImVec4(0.10f, 0.49f, 0.86f, 1.0f));
    ImGui::PushStyleColor(
        ImGuiCol_ButtonActive,
        ImVec4(0.06f, 0.29f, 0.54f, 1.0f));
    constexpr ImGuiWindowFlags productMenuFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::BeginPopup(
            "メニュー###UGCProductMenu",
            productMenuFlags)) {
        if (shouldToggleMenu && wasMenuOpen) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::SetWindowFontScale(1.15f);
        ImGui::TextUnformatted("メニュー");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::Separator();
        constexpr float menuButtonWidth = 248.0f;
        constexpr float menuButtonHeight = 46.0f;
        if (mUGCEditorTutorial.IsActive()) {
            ImGui::BeginDisabled();
        }
        ImGui::SetItemDefaultFocus();
        if (ImGui::Button(
                "保存・開く",
                ImVec2(menuButtonWidth, menuButtonHeight))) {
            mShouldOpenUGCWorkManagement = true;
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(
                "完成チェック",
                ImVec2(menuButtonWidth, menuButtonHeight))) {
            StartUGCVerification();
            ImGui::CloseCurrentPopup();
        }
        if (mUGCEditorTutorial.IsActive()) {
            ImGui::EndDisabled();
        }

        ImGui::Spacing();
        ImGui::PushStyleColor(
            ImGuiCol_Button,
            ImVec4(0.22f, 0.25f, 0.32f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonHovered,
            ImVec4(0.34f, 0.39f, 0.48f, 1.0f));
        ImGui::PushStyleColor(
            ImGuiCol_ButtonActive,
            ImVec4(0.17f, 0.20f, 0.27f, 1.0f));
        if (ImGui::Button(
                "タイトルへ戻る",
                ImVec2(menuButtonWidth, menuButtonHeight))) {
            if (mUGCWorkPanel.HasUnsavedChanges()) {
                ImGui::OpenPopup(
                    "保存せずタイトルへ戻りますか###UGCExitConfirmation");
            } else {
                mContext.game->ExitUGCMode();
            }
        }
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopupModal(
                "保存せずタイトルへ戻りますか###UGCExitConfirmation",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextUnformatted(
                "保存していない変更があります。タイトルへ戻りますか？");
            ImGui::TextDisabled("保存していない変更は失われます。");
            ImGui::Spacing();
            if (ImGui::Button(
                    "保存せず戻る",
                    ImVec2(150.0f, 42.0f))) {
                mContext.game->ExitUGCMode();
            }
            ImGui::SameLine();
            if (ImGui::Button("やめる", ImVec2(120.0f, 42.0f)) ||
                ImGui::IsKeyPressed(ImGuiKey_Escape) ||
                ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight)) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        const bool shouldCloseMenu =
            ImGui::IsKeyPressed(ImGuiKey_Escape) ||
            ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight);
        if (shouldCloseMenu &&
            !ImGui::IsPopupOpen(
                "保存せずタイトルへ戻りますか###UGCExitConfirmation")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(5);
    ImGui::End();
    ImGui::PopStyleVar();
    DrawUGCViewport(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight,
        gameViewportMin,
        gameViewportMax);
    const bool isWorkManagementOpen = IsUGCWorkManagementOpen();
    if (!isWorkManagementOpen) {
        DrawUGCGridOverlay();
        DrawUGCStackBadges();
        DrawUGCPlacementPreview();
        DrawUGCPreviewOverlay();
    }

    if (mStageAddActorPanel.IsPlacementActive()) {
        mStageAddActorPanel.UpdatePlacement();
    } else {
        const bool isChoosingSwitchTarget =
            mUGCConnectionSwitchRef.has_value();
        const bool allowsSelectionInteraction =
            !mIsUGCEraserMode && !isChoosingSwitchTarget;
        mSelectionController.SetBoxSelectionEnabled(
            allowsSelectionInteraction);
        mSelectionController.Update();
        if (!mIsUGCEraserMode &&
            !isChoosingSwitchTarget &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
            !ImGui::GetIO().KeyShift) {
            SyncUGCEditLayerToPickedActor();
        }
        if (allowsSelectionInteraction &&
            !mSelectionController.IsBoxSelectionGestureActive()) {
            UpdateUGCSelectionDrag();
        }
        if (mIsUGCEraserMode && !isChoosingSwitchTarget &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            const auto tryDeletePickedPlanetOnly = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    return false;
                }

                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) == nullptr ||
                    pickedRef->type != StageActorType::Planet) {
                    return false;
                }

                return mEditCommandController.DeletePlanetOnly(
                    pickedRef->yamlIndex);
            };
            const auto tryDeletePickedActorOnCurrentLayer = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
                    !mContext.game) {
                    return false;
                }

                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) != nullptr) {
                    return false;
                }




                Platform* pickedPlatform = dynamic_cast<Platform*>(pickedActor);
                if (pickedPlatform && pickedPlatform->GetIsUGCGenerated()) {
                    return false;
                }
                const float gridSize = mContext.game->GetUGCGridSize();
                const int actorLayer = static_cast<int>(std::round(
                    pickedActor->GetPos().y / gridSize));
                if (actorLayer != mUGCEditLayer) {
                    return false;
                }

                return mEditCommandController.DeleteSelectedKeys({
                    StageActorQuery::MakeKey(*pickedRef)});
            };

            const auto tryDeletePickedActorAsHighestFallback = [&]() {
                if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    return false;
                }
                Actor* pickedActor = mSelectionController.GetPickedActor();
                const std::optional<StageActorRef>& pickedRef =
                    mSelectionController.GetPickedActorRef();
                Platform* pickedPlatform =
                    dynamic_cast<Platform*>(pickedActor);
                if (!pickedActor || !pickedRef ||
                    dynamic_cast<Planet*>(pickedActor) != nullptr ||
                    (pickedPlatform && pickedPlatform->GetIsUGCGenerated())) {
                    return false;
                }
                return mEditCommandController.DeleteSelectedKeys({
                    StageActorQuery::MakeKey(*pickedRef)});
            };

            if (tryDeletePickedPlanetOnly()) {
                mUGCStatus = "惑星だけを消しました";
                mUGCEditorTutorial.RecordErase(true);
            } else if (tryDeletePickedActorOnCurrentLayer()) {
                mUGCStatus = "今のだんのものを消しました";
                mUGCEditorTutorial.RecordErase(true);
            } else if (mStageAddActorPanel.TryEraseUGCPlatformCell()) {
                mUGCStatus = "足場を1マス消しました";
                mUGCEditorTutorial.RecordErase(true);
            } else if (tryDeletePickedActorAsHighestFallback()) {
                mUGCStatus = "いちばん上のものを消しました";
                mUGCEditorTutorial.RecordErase(true);
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                       !mSelectionController.GetSelectedKeys().empty()) {
                const std::optional<StageActorRef>& selectedRef =
                    mSelectionController.GetPickedActorRef();
                if (!selectedRef ||
                    selectedRef->type != StageActorType::Planet) {
                    const std::unordered_set<std::string> selectedKeys =
                        mSelectionController.GetSelectedKeys();
                    if (mEditCommandController.DeleteSelectedKeys(selectedKeys)) {
                        mUGCStatus = "選んだものを消しました";
                        mUGCEditorTutorial.RecordErase(true);
                    }
                }
            }
        }
    }
    mSelectionController.ApplyEditorSelectionFlags();
    if (!isWorkManagementOpen) {
        mSelectionController.DrawBoxSelectionRect();
        DrawUGCSwitchConnectionLines();
        DrawUGCUnconnectedSwitchWarnings();
        DrawUGCTransformControls();
    }
    DrawUGCEditorTutorial();
    if (!isWorkManagementOpen && mContext.uiRenderer) {
        mContext.uiRenderer->DrawUGCForegroundCustomUI(
            gameViewportMin,
            ImVec2(
                gameViewportMax.x - gameViewportMin.x,
                gameViewportMax.y - gameViewportMin.y));
    }
    DrawUGCWorkManagement();
}

void DebugUIRenderer::DrawUGCSwitchConnectionLines()
{
    Platform* selectedPlatform = dynamic_cast<Platform*>(
        mSelectionController.GetSingleSelectedActor());
    if (!selectedPlatform || !mContext.game ||
        !mContext.game->GetCurrentStage()) {
        return;
    }

    const std::string& selectedPlatformId =
        selectedPlatform->GetPlatformId();
    const std::optional<StageActorRef>& selectedRef =
        mSelectionController.GetPickedActorRef();
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const auto drawConnection =
        [this, drawList](Platform* switchPlatform, Platform* targetPlatform) {
            if (!switchPlatform || !targetPlatform) {
                return;
            }

            ImVec2 switchScreenPosition;
            ImVec2 targetScreenPosition;
            if (!mSelectionController.TryWorldToScreenPoint(
                    switchPlatform->GetPos(), switchScreenPosition) ||
                !mSelectionController.TryWorldToScreenPoint(
                    targetPlatform->GetPos(), targetScreenPosition)) {
                return;
            }

            drawList->AddLine(
                switchScreenPosition,
                targetScreenPosition,
                IM_COL32(5, 15, 24, 220),
                6.0f);
            drawList->AddLine(
                switchScreenPosition,
                targetScreenPosition,
                IM_COL32(70, 225, 255, 245),
                3.0f);
            drawList->AddCircleFilled(
                switchScreenPosition,
                6.0f,
                IM_COL32(70, 225, 255, 255));
            drawList->AddCircleFilled(
                targetScreenPosition,
                6.0f,
                IM_COL32(255, 225, 90, 255));
        };

    const auto findPlatformById =
        [this](const std::string& platformId, int preferredYamlIndex) {
        if (platformId.empty()) {
            return static_cast<Platform*>(nullptr);
        }
        Platform* matchingPlatform = nullptr;
        for (Planet* planet :
             mContext.game->GetCurrentStage()->GetPlanets()) {
            if (!planet) {
                continue;
            }
            for (Platform* platform : planet->GetPlatforms()) {
                if (platform && platform->GetPlatformId() == platformId) {
                    if (platform->GetStageYamlIndex() ==
                        preferredYamlIndex) {
                        return platform;
                    }
                    if (!matchingPlatform) {
                        matchingPlatform = platform;
                    }
                }
            }
        }
        return matchingPlatform;
    };

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* switchPlatform : planet->GetPlatforms()) {
            PlatformPressureSwitchComponent* pressureSwitch =
                switchPlatform
                    ? switchPlatform->GetPressureSwitchComponent()
                    : nullptr;
            if (!pressureSwitch) {
                continue;
            }

            for (const std::string& targetPlatformId :
                 pressureSwitch->GetTargetPlatformIds()) {
                const bool isSelectedSwitch =
                    switchPlatform == selectedPlatform;
                const bool isSelectedTarget =
                    !selectedPlatformId.empty() &&
                    targetPlatformId == selectedPlatformId;
                if (!isSelectedSwitch && !isSelectedTarget) {
                    continue;
                }
                drawConnection(
                    switchPlatform,
                    findPlatformById(targetPlatformId, -1));
            }
        }
    }

    if (!selectedRef) {
        return;
    }

    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* switchPlatform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* groupSwitch =
                switchPlatform
                    ? switchPlatform->GetLatchedGroupSwitchComponent()
                    : nullptr;
            if (!groupSwitch || groupSwitch->GetGroupId().empty()) {
                continue;
            }

            std::vector<PlatformRevealTarget> groupTargets;
            for (Planet* groupPlanet :
                 mContext.game->GetCurrentStage()->GetPlanets()) {
                if (!groupPlanet) {
                    continue;
                }
                for (Platform* groupMember : groupPlanet->GetPlatforms()) {
                    PlatformLatchedGroupSwitchComponent* memberComponent =
                        groupMember
                            ? groupMember->GetLatchedGroupSwitchComponent()
                            : nullptr;
                    if (!memberComponent ||
                        memberComponent->GetGroupId() !=
                            groupSwitch->GetGroupId()) {
                        continue;
                    }
                    groupTargets.insert(
                        groupTargets.end(),
                        memberComponent->GetRevealTargets().begin(),
                        memberComponent->GetRevealTargets().end());
                }
            }

            PlatformLatchedGroupSwitchComponent* selectedGroupSwitch =
                selectedPlatform->GetLatchedGroupSwitchComponent();
            const bool isSelectedGroup = selectedGroupSwitch &&
                selectedGroupSwitch->GetGroupId() ==
                    groupSwitch->GetGroupId();
            for (const PlatformRevealTarget& target : groupTargets) {
                const bool isSelectedTarget =
                    (!target.platformId.empty() &&
                     target.platformId == selectedPlatformId) ||
                    (target.platformId.empty() &&
                     target.sequenceName == selectedRef->sequenceName &&
                     target.yamlIndex == selectedRef->yamlIndex);
                if (!isSelectedGroup && !isSelectedTarget) {
                    continue;
                }

                Platform* targetPlatform =
                    !target.platformId.empty()
                        ? findPlatformById(
                              target.platformId,
                              target.yamlIndex)
                        : dynamic_cast<Platform*>(
                              StageActorQuery::FindActorByRef(
                                  mContext.game->GetCurrentStage(),
                                  StageActorRef{
                                      StageActorType::Platform,
                                      target.yamlIndex,
                                      target.sequenceName,
                                      ""}));
                drawConnection(switchPlatform, targetPlatform);
            }
        }
    }
}

void DebugUIRenderer::DrawUGCUnconnectedSwitchWarnings()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    std::unordered_set<std::string> connectedGroupIds;
    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            PlatformLatchedGroupSwitchComponent* groupSwitch =
                platform
                    ? platform->GetLatchedGroupSwitchComponent()
                    : nullptr;
            if (!groupSwitch || groupSwitch->GetGroupId().empty()) {
                continue;
            }
            const bool hasConnectedTarget =
                !groupSwitch->GetRevealTargets().empty() ||
                !groupSwitch->GetHideTargets().empty();
            if (hasConnectedTarget) {
                connectedGroupIds.insert(groupSwitch->GetGroupId());
            }
        }
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (Planet* planet : mContext.game->GetCurrentStage()->GetPlanets()) {
        if (!planet) {
            continue;
        }
        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            const PlatformPressureSwitchComponent* pressureSwitch =
                platform->GetPressureSwitchComponent();
            const bool isUnconnectedPressureSwitch = pressureSwitch &&
                pressureSwitch->GetTargetPlatformIds().empty() &&
                pressureSwitch->GetTargetEnemyRefs().empty() &&
                pressureSwitch->GetHideTargets().empty();

            const PlatformLatchedGroupSwitchComponent* groupSwitch =
                platform->GetLatchedGroupSwitchComponent();
            const bool isUnconnectedGroupSwitch = groupSwitch &&
                (groupSwitch->GetGroupId().empty() ||
                 !connectedGroupIds.contains(groupSwitch->GetGroupId()));
            if (!isUnconnectedPressureSwitch &&
                !isUnconnectedGroupSwitch) {
                continue;
            }

            ImVec2 screenPosition;
            if (mSelectionController.TryWorldToScreenPoint(
                    platform->GetPos(), screenPosition)) {
                DrawUGCWarningTriangle(drawList, screenPosition);
            }
        }
    }
}

bool DebugUIRenderer::TryIntersectUGCDragPlane(
    const glm::vec3& rayFrom,
    const glm::vec3& rayTo,
    glm::vec3& outIntersection) const
{
    const glm::vec3 rayDirection = rayTo - rayFrom;
    const float denominator = glm::dot(
        rayDirection, mUGCSelectionDragPlaneNormal);
    if (std::abs(denominator) <= 0.000001f) {
        return false;
    }

    const float rayParameter = glm::dot(
        mUGCSelectionDragPlanePoint - rayFrom,
        mUGCSelectionDragPlaneNormal) / denominator;
    if (rayParameter < 0.0f || rayParameter > 1.0f) {
        return false;
    }
    outIntersection = rayFrom + rayDirection * rayParameter;
    return true;
}

void DebugUIRenderer::UpdateUGCSelectionDrag()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    const bool isMouseDown =
        ImGui::IsMouseDown(ImGuiMouseButton_Left);
    if (!mIsUGCSelectionDragging) {
        if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::GetIO().WantCaptureMouse ||
            mSelectionController.GetSelectedActorCount() == 0) {
            return;
        }

        const std::vector<StageActorInstance> selectedInstances =
            mSelectionController.CollectSelectedActorInstances();
        if (selectedInstances.empty()) {
            return;
        }

        glm::vec3 rayFrom;
        glm::vec3 rayTo;
        if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo)) {
            return;
        }

        const CameraPose cameraPose =
            mContext.game->GetCameraSystem()->GetDebugCameraPose();
        glm::vec3 planeNormal = cameraPose.position - cameraPose.target;
        if (glm::length(planeNormal) <= 0.000001f) {
            return;
        }
        mUGCSelectionDragPlaneNormal = glm::normalize(planeNormal);
        const glm::vec3 selectionCenter =
            mSelectionController.CalculateSelectedActorsCenter();
        mUGCSelectionDragPlanePoint = selectionCenter;

        glm::vec3 intersection;
        if (!TryIntersectUGCDragPlane(rayFrom, rayTo, intersection)) {
            return;
        }

        mIsUGCSelectionDragging = true;
        mIsUGCMovingPlatformDestinationDrag =
            mSelectionController.IsMovingPlatformDestinationSelected();
        mHasUGCSelectionDragMoved = false;
        mUGCSelectionDragOffset =
            selectionCenter - intersection;
        mUGCSelectionDragInitialCenter = selectionCenter;
        mUGCSelectionDragAppliedDelta = glm::vec3(0.0f);
        mUGCSelectionDragSavedDelta = glm::vec3(0.0f);
        mUGCSelectionDragActorRefs.clear();
        mUGCSelectionDragActorRefs.reserve(selectedInstances.size());
        for (const StageActorInstance& selectedInstance : selectedInstances) {
            mUGCSelectionDragActorRefs.emplace_back(selectedInstance.ref);
        }
        return;
    }

    if (!isMouseDown) {
        if (mHasUGCSelectionDragMoved) {
            glm::vec3 totalDelta = mUGCSelectionDragAppliedDelta;
            if (mIsUGCMovingPlatformDestinationDrag) {
                const glm::vec3 finalDestinationCenter =
                    mSelectionController
                        .CalculateSelectedMovingPlatformDestinationsCenter();
                totalDelta = finalDestinationCenter -
                    mUGCSelectionDragInitialCenter -
                    mUGCSelectionDragSavedDelta;
            }
            bool updatedUGCPlatform = false;
            if (mIsUGCMovingPlatformDestinationDrag) {
                constexpr float minimumTranslationLength = 0.0001f;
                updatedUGCPlatform = glm::length(totalDelta) <
                    minimumTranslationLength;
                if (!updatedUGCPlatform) {
                    updatedUGCPlatform = mStageAddActorPanel
                        .TryTranslateUGCMovingPlatformDestinations(
                            mUGCSelectionDragActorRefs, totalDelta);
                }
                if (!updatedUGCPlatform) {
                    mSelectionController
                        .MoveSelectedMovingPlatformDestinationsByDelta(
                            -totalDelta);
                }
            } else {
                mStageActorYamlWriter.SaveEditorAuthoredTransforms();
                updatedUGCPlatform =
                    mStageAddActorPanel.TryTranslateUGCPlatformCells(
                        mUGCSelectionDragActorRefs, totalDelta);
            }
            if (updatedUGCPlatform) {
                mSelectionController.Clear();
            }
            mUGCStatus = updatedUGCPlatform ||
                    !mIsUGCMovingPlatformDestinationDrag
                ? "移動しました"
                : "移動先を動かせませんでした";
            if (updatedUGCPlatform ||
                !mIsUGCMovingPlatformDestinationDrag) {
                mUGCEditorTutorial.RecordSelectionMove();
            }
        }
        mIsUGCSelectionDragging = false;
        mIsUGCMovingPlatformDestinationDrag = false;
        mHasUGCSelectionDragMoved = false;
        mUGCSelectionDragAppliedDelta = glm::vec3(0.0f);
        mUGCSelectionDragSavedDelta = glm::vec3(0.0f);
        mUGCSelectionDragActorRefs.clear();
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    glm::vec3 intersection;
    if (!mSelectionController.TryCreateMouseRay(rayFrom, rayTo) ||
        !TryIntersectUGCDragPlane(rayFrom, rayTo, intersection)) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 unsnappedTarget =
        intersection + mUGCSelectionDragOffset;
    const glm::vec3 unsnappedDelta =
        unsnappedTarget - mUGCSelectionDragInitialCenter;
    const glm::vec3 snappedTarget =
        mUGCSelectionDragInitialCenter + glm::vec3(
            std::round(unsnappedDelta.x / gridSize) * gridSize,
            std::round(unsnappedDelta.y / gridSize) * gridSize,
            std::round(unsnappedDelta.z / gridSize) * gridSize);
    const glm::vec3 currentCenter = mIsUGCMovingPlatformDestinationDrag
        ? mSelectionController
              .CalculateSelectedMovingPlatformDestinationsCenter()
        : mSelectionController.CalculateSelectedActorsCenter();
    const glm::vec3 movementDelta = snappedTarget - currentCenter;
    if (glm::length(movementDelta) <= 0.000001f) {
        return;
    }

    if (!mHasUGCSelectionDragMoved) {
        mEditCommandController.PushUndo();
        mHasUGCSelectionDragMoved = true;
    }
    if (mIsUGCMovingPlatformDestinationDrag) {
        mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(
            movementDelta);
    } else {
        mSelectionController.MoveSelectedActorsByDelta(movementDelta);
    }
    mUGCSelectionDragAppliedDelta += movementDelta;
}

void DebugUIRenderer::DrawUGCViewport(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight,
    const ImVec2& viewportMin,
    const ImVec2& viewportMax)
{
    const ImVec2 imageAreaMin = viewportMin;
    ImDrawList* background = ImGui::GetBackgroundDrawList();
    background->AddRectFilled(imageAreaMin, viewportMax, IM_COL32(20, 20, 24, 255));
    mContext.gameViewport = {};
    if (gameViewTexture == 0 || gameViewWidth <= 0 || gameViewHeight <= 0) {
        return;
    }

    const float availableWidth = std::max(1.0f, viewportMax.x - imageAreaMin.x);
    const float availableHeight = std::max(1.0f, viewportMax.y - imageAreaMin.y);
    const float sourceAspect = static_cast<float>(gameViewWidth) /
        static_cast<float>(gameViewHeight);
    float imageWidth = availableWidth;
    float imageHeight = imageWidth / sourceAspect;
    if (imageHeight > availableHeight) {
        imageHeight = availableHeight;
        imageWidth = imageHeight * sourceAspect;
    }
    const ImVec2 imageMin(
        imageAreaMin.x + (availableWidth - imageWidth) * 0.5f,
        imageAreaMin.y + (availableHeight - imageHeight) * 0.5f);
    const ImVec2 imageMax(imageMin.x + imageWidth, imageMin.y + imageHeight);
    background->AddImage(
        static_cast<ImTextureID>(gameViewTexture),
        imageMin,
        imageMax,
        ImVec2(0.0f, 1.0f),
        ImVec2(1.0f, 0.0f));

    mContext.gameViewport = {
        imageMin.x,
        imageMin.y,
        imageWidth,
        imageHeight,
        gameViewWidth,
        gameViewHeight};
}

void DebugUIRenderer::DrawUGCPreviewOverlay()
{
    if (!mContext.game || !mContext.gameViewport.IsValid()) {
        return;
    }
    const unsigned int previewTexture =
        mContext.game->GetUGCPreviewTexture();
    if (previewTexture == 0) {
        return;
    }

    const float maximumPreviewWidth = std::max(
        1.0f,
        mContext.gameViewport.width * 0.5f);
    const float minimumPreviewWidth = std::min(180.0f, maximumPreviewWidth);
    if (!mHasInitializedUGCPreviewWidth) {


        mUGCPreviewWidth = glm::clamp(
            maximumPreviewWidth * (2.0f / 3.0f),
            minimumPreviewWidth,
            maximumPreviewWidth);
        mUGCPreviewResizeStartWidth = mUGCPreviewWidth;
        mHasInitializedUGCPreviewWidth = true;
    }
    UILoadSystem::CustomElement* previewElement = nullptr;
    if (mContext.uiRenderer &&
        mContext.uiRenderer->GetUILoadSystem()) {
        for (UILoadSystem::CustomElement& element :
             mContext.uiRenderer->GetUILoadSystem()->GetCustomElements()) {
            if (element.screen == "ugc" && element.id == "preview") {
                previewElement = &element;
                break;
            }
        }
    }

    float previewWidth = glm::clamp(
        mUGCPreviewWidth,
        minimumPreviewWidth,
        maximumPreviewWidth);
    float previewHeight = previewWidth * 9.0f / 16.0f;
    ImVec2 previewMin(
        mContext.gameViewport.x +
            mContext.gameViewport.width - previewWidth - 14.0f,
        mContext.gameViewport.y +
            mContext.gameViewport.height - previewHeight - 14.0f);
    if (previewElement) {
        previewWidth = std::max(
            1.0f,
            mContext.gameViewport.width *
                previewElement->widthRatio);
        previewHeight = std::max(
            1.0f,
            mContext.gameViewport.width *
                previewElement->heightRatio);
        previewMin = ImVec2(
            mContext.gameViewport.x +
                mContext.gameViewport.width *
                    previewElement->xRatio,
            mContext.gameViewport.y +
                mContext.gameViewport.width *
                    previewElement->yRatio);
    }
    mUGCPreviewWidth = previewWidth;
    const ImVec2 framebufferScale = ImGui::GetIO().DisplayFramebufferScale;
    constexpr float previewQualityScale = 1.5f;
    mContext.game->SetUGCPreviewRenderSize(
        static_cast<int>(std::ceil(
            previewWidth * framebufferScale.x * previewQualityScale)),
        static_cast<int>(std::ceil(
            previewHeight * framebufferScale.y * previewQualityScale)));
    const ImVec2 previewMax(
        previewMin.x + previewWidth,
        previewMin.y + previewHeight);
    const bool isAdjustingUGCUI =
        mActiveSection == EditorSection::UserInterface;
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->AddRectFilled(
        ImVec2(previewMin.x - 4.0f, previewMin.y - 24.0f),
        ImVec2(previewMax.x + 4.0f, previewMax.y + 4.0f),
        IM_COL32(15, 18, 25, 245), 5.0f);
    drawList->AddText(
        ImVec2(previewMin.x, previewMin.y - 20.0f),
        IM_COL32(235, 240, 255, 255),
        "3Dプレビュー　← → 回転");
    drawList->AddImage(
        static_cast<ImTextureID>(previewTexture),
        previewMin,
        previewMax,
        ImVec2(0.0f, 1.0f),
        ImVec2(1.0f, 0.0f));
    DrawUGCPreviewLayerGuides(previewMin, previewMax, drawList);
    drawList->AddRect(
        previewMin, previewMax,
        IM_COL32(140, 190, 255, 230),
        3.0f, 0, 2.0f);

    constexpr float resizeHandleSize = 22.0f;
    const ImVec2 resizeHandleMin(
        previewMin.x - resizeHandleSize * 0.5f,
        previewMax.y - resizeHandleSize * 0.5f);
    ImGui::SetNextWindowPos(resizeHandleMin, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(resizeHandleSize, resizeHandleSize),
        ImGuiCond_Always);
    const ImGuiWindowFlags resizeWindowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        (isAdjustingUGCUI ? ImGuiWindowFlags_NoInputs : 0);
    ImGui::Begin(
        "3Dプレビューサイズ変更###UGCPreviewResize",
        nullptr,
        resizeWindowFlags);
    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::InvisibleButton(
        "###UGCPreviewResizeHandle",
        ImVec2(resizeHandleSize, resizeHandleSize));
    if (ImGui::IsItemActivated()) {
        mUGCPreviewResizeStartWidth = mUGCPreviewWidth;
    }
    if (ImGui::IsItemActive() && !isAdjustingUGCUI) {
        const float horizontalDrag =
            ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
        mUGCPreviewWidth = glm::clamp(
            mUGCPreviewResizeStartWidth - horizontalDrag,
            minimumPreviewWidth,
            maximumPreviewWidth);
        if (previewElement) {
            const float previousWidthRatio =
                previewElement->widthRatio;
            const float previousHeightRatio =
                previewElement->heightRatio;
            const float resizedWidthRatio =
                mUGCPreviewWidth /
                std::max(mContext.gameViewport.width, 1.0f);
            const float resizedHeightRatio =
                resizedWidthRatio * 9.0f / 16.0f;
            previewElement->xRatio +=
                previousWidthRatio - resizedWidthRatio;
            previewElement->yRatio +=
                previousHeightRatio - resizedHeightRatio;
            previewElement->widthRatio = resizedWidthRatio;
            previewElement->heightRatio = resizedHeightRatio;
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
    } else if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
    }
    ImGui::End();

    drawList->AddTriangleFilled(
        ImVec2(previewMin.x, previewMax.y),
        ImVec2(previewMin.x + 13.0f, previewMax.y),
        ImVec2(previewMin.x, previewMax.y - 13.0f),
        IM_COL32(175, 215, 255, 245));
}

void DebugUIRenderer::DrawUGCPreviewLayerGuides(
    const ImVec2& previewMin,
    const ImVec2& previewMax,
    ImDrawList* drawList)
{
    if (!drawList || !mContext.game ||
        !mContext.game->GetCameraSystem()) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    float minimumX = std::numeric_limits<float>::max();
    float maximumX = std::numeric_limits<float>::lowest();
    float minimumZ = std::numeric_limits<float>::max();
    float maximumZ = std::numeric_limits<float>::lowest();
    std::set<int> occupiedLayers;

    YAML::Node stageYaml;
    if (StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        const YAML::Node cells = stageYaml["ugcPlatformCells"];
        if (cells && cells.IsSequence()) {
            for (const YAML::Node& cell : cells) {
                const YAML::Node gridPosition = cell["gridPosition"];
                if (!gridPosition || !gridPosition.IsSequence() ||
                    gridPosition.size() < 3 || !cell["gridSize"] ||
                    std::abs(cell["gridSize"].as<float>(gridSize) - gridSize) >=
                        0.0001f) {
                    continue;
                }
                const int x = gridPosition[0].as<int>();
                const int layer = gridPosition[1].as<int>();
                const int z = gridPosition[2].as<int>();
                occupiedLayers.insert(layer);
                minimumX = std::min(minimumX, static_cast<float>(x) * gridSize);
                maximumX = std::max(
                    maximumX, static_cast<float>(x + 1) * gridSize);
                minimumZ = std::min(minimumZ, static_cast<float>(z) * gridSize);
                maximumZ = std::max(
                    maximumZ, static_cast<float>(z + 1) * gridSize);
            }
        }
    }

    if (minimumX > maximumX || minimumZ > maximumZ) {
        const CameraPose editorPose =
            mContext.game->GetCameraSystem()->GetDebugCameraPose();
        constexpr float emptyStageHalfExtent = 6.0f;
        minimumX = editorPose.target.x - emptyStageHalfExtent;
        maximumX = editorPose.target.x + emptyStageHalfExtent;
        minimumZ = editorPose.target.z - emptyStageHalfExtent;
        maximumZ = editorPose.target.z + emptyStageHalfExtent;
    } else {
        minimumX -= gridSize;
        maximumX += gridSize;
        minimumZ -= gridSize;
        maximumZ += gridSize;
    }

    const CameraPose editorPose =
        mContext.game->GetCameraSystem()->GetDebugCameraPose();
    const glm::vec3 basePreviewDirection = glm::normalize(
        glm::vec3(1.0f, 0.75f, 1.0f));
    const float previewYaw = mContext.game->GetUGCPreviewYawRadians();
    const float previewYawCosine = std::cos(previewYaw);
    const float previewYawSine = std::sin(previewYaw);
    const glm::vec3 previewDirection = glm::normalize(glm::vec3(
        basePreviewDirection.x * previewYawCosine +
            basePreviewDirection.z * previewYawSine,
        mContext.game->GetIsUGCPreviewViewedFromBelow()
            ? -basePreviewDirection.y
            : basePreviewDirection.y,
        -basePreviewDirection.x * previewYawSine +
            basePreviewDirection.z * previewYawCosine));
    constexpr float previewFieldOfViewDegrees = 55.0f;
    const float editorViewDistance = mContext.game->GetIsUGCOrthographicView()
        ? mContext.game->GetUGCOrthographicHalfHeight() /
              std::tan(glm::radians(previewFieldOfViewDegrees) * 0.5f)
        : glm::length(editorPose.position - editorPose.target);
    const float previewDistance = glm::clamp(
        editorViewDistance * 0.45f, 3.0f, 100.0f);
    glm::vec3 previewTarget = editorPose.target;
    previewTarget.y = mContext.game->GetUGCPreviewFocusY();
    const glm::vec3 previewPosition =
        previewTarget + previewDirection * previewDistance;
    const glm::vec3 previewUp =
        mContext.game->GetIsUGCPreviewViewedFromBelow()
            ? glm::vec3(0.0f, -1.0f, 0.0f)
            : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(
        previewPosition,
        previewTarget,
        previewUp);
    const glm::mat4 projection = glm::perspective(
        glm::radians(previewFieldOfViewDegrees),
        16.0f / 9.0f,
        0.1f,
        1000.0f);

    const auto projectViewPoint = [&](const glm::vec3& viewPoint,
                                      ImVec2& outScreenPoint) {
        const glm::vec4 clip = projection * glm::vec4(viewPoint, 1.0f);
        if (clip.w <= 0.0001f) {
            return false;
        }
        const glm::vec3 ndc = glm::vec3(clip) / clip.w;
        outScreenPoint.x = previewMin.x +
            (ndc.x * 0.5f + 0.5f) * (previewMax.x - previewMin.x);
        outScreenPoint.y = previewMin.y +
            (1.0f - (ndc.y * 0.5f + 0.5f)) *
                (previewMax.y - previewMin.y);
        return true;
    };
    const auto projectPoint = [&](const glm::vec3& worldPoint,
                                  ImVec2& outScreenPoint) {
        const glm::vec3 viewPoint = glm::vec3(
            view * glm::vec4(worldPoint, 1.0f));
        return projectViewPoint(viewPoint, outScreenPoint);
    };
    const auto clipLineToNearPlane = [](
        glm::vec3& viewFrom,
        glm::vec3& viewTo) {
        constexpr float nearPlaneZ = -0.1001f;
        const bool isFromVisible = viewFrom.z <= nearPlaneZ;
        const bool isToVisible = viewTo.z <= nearPlaneZ;
        if (!isFromVisible && !isToVisible) {
            return false;
        }
        if (isFromVisible && isToVisible) {
            return true;
        }

        glm::vec3& hiddenEndpoint = isFromVisible ? viewTo : viewFrom;
        const glm::vec3& visibleEndpoint = isFromVisible ? viewFrom : viewTo;
        const float distanceRatio =
            (nearPlaneZ - hiddenEndpoint.z) /
            (visibleEndpoint.z - hiddenEndpoint.z);
        hiddenEndpoint +=
            (visibleEndpoint - hiddenEndpoint) * distanceRatio;
        return true;
    };
    const auto drawWorldLine = [&](const glm::vec3& from,
                                   const glm::vec3& to,
                                   ImU32 color,
                                   float thickness) {
        glm::vec3 viewFrom = glm::vec3(view * glm::vec4(from, 1.0f));
        glm::vec3 viewTo = glm::vec3(view * glm::vec4(to, 1.0f));
        if (!clipLineToNearPlane(viewFrom, viewTo)) {
            return;
        }

        ImVec2 screenFrom;
        ImVec2 screenTo;
        if (projectViewPoint(viewFrom, screenFrom) &&
            projectViewPoint(viewTo, screenTo)) {
            drawList->AddLine(screenFrom, screenTo, color, thickness);
        }
    };
    const auto drawLayerLabel = [&](int layer, ImU32 color) {
        ImVec2 labelPosition;
        if (!projectPoint(
                glm::vec3(
                    minimumX,
                    static_cast<float>(layer) * gridSize,
                    minimumZ),
                labelPosition)) {
            return;
        }
        const std::string label = std::to_string(layer + 1) + "だん";
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());


        labelPosition.x = previewMin.x + 7.0f;
        labelPosition.y = glm::clamp(
            labelPosition.y,
            previewMin.y + 3.0f,
            previewMax.y - textSize.y - 3.0f);
        const ImVec2 labelMin(labelPosition.x - 4.0f, labelPosition.y - 3.0f);
        const ImVec2 labelMax(
            labelPosition.x + textSize.x + 4.0f,
            labelPosition.y + textSize.y + 3.0f);
        drawList->AddRectFilled(
            labelMin, labelMax, IM_COL32(10, 14, 22, 205), 3.0f);
        drawList->AddText(labelPosition, color, label.c_str());
    };

    drawList->PushClipRect(previewMin, previewMax, true);
    for (const int layer : occupiedLayers) {
        if (layer == mUGCEditLayer) {
            continue;
        }
        drawLayerLabel(layer, IM_COL32(160, 225, 255, 240));
    }

    const float currentLayerY =
        static_cast<float>(mUGCEditLayer) * gridSize;
    float visualGridSize = gridSize;
    constexpr int maximumGridLineCount = 24;
    while ((maximumX - minimumX) / visualGridSize > maximumGridLineCount ||
           (maximumZ - minimumZ) / visualGridSize > maximumGridLineCount) {
        visualGridSize *= 2.0f;
    }
    constexpr ImU32 currentGridColor = IM_COL32(255, 202, 72, 105);
    for (float x = minimumX; x <= maximumX + 0.001f; x += visualGridSize) {
        drawWorldLine(
            {x, currentLayerY, minimumZ},
            {x, currentLayerY, maximumZ},
            currentGridColor,
            1.0f);
    }
    for (float z = minimumZ; z <= maximumZ + 0.001f; z += visualGridSize) {
        drawWorldLine(
            {minimumX, currentLayerY, z},
            {maximumX, currentLayerY, z},
            currentGridColor,
            1.0f);
    }
    drawLayerLabel(mUGCEditLayer, IM_COL32(255, 220, 105, 255));
    drawList->PopClipRect();
}

void DebugUIRenderer::DrawUGCPlacementPreview()
{
    if (!mContext.gameViewport.IsValid()) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    std::string placementInstruction;
    if (mStageAddActorPanel.IsPlacementActive()) {
        const std::string& placementStatus =
            mStageAddActorPanel.GetPlacementStatus();
        if (!placementStatus.empty()) {
            placementInstruction =
                mStageAddActorPanel.GetPlacementDisplayName() + "\n" +
                placementStatus;
        }
    } else if (mUGCConnectionSwitchRef) {
        placementInstruction = mUGCSwitchConnectionAction ==
                UGCSwitchConnectionAction::Connect
            ? "スイッチ：表示する足場\n"
              "対応する足場をクリックしてください"
            : "スイッチ：つながりを解除\n"
              "解除する足場をクリックしてください";
    }

    if (!placementInstruction.empty()) {
        const ImVec2 instructionPosition(
            mContext.gameViewport.x + 18.0f,
            mContext.gameViewport.y + 18.0f);
        ImFont* instructionFont = ImGui::GetFont();
        const float instructionFontSize = ImGui::GetFontSize() * 2.0f;
        const ImVec2 instructionSize = instructionFont->CalcTextSizeA(
            instructionFontSize,
            std::numeric_limits<float>::max(),
            0.0f,
            placementInstruction.c_str());
        constexpr float instructionPadding = 16.0f;
        drawList->AddRectFilled(
            ImVec2(
                instructionPosition.x - instructionPadding,
                instructionPosition.y - instructionPadding),
            ImVec2(
                instructionPosition.x + instructionSize.x +
                    instructionPadding,
                instructionPosition.y + instructionSize.y +
                    instructionPadding),
            IM_COL32(8, 15, 27, 220),
            5.0f);
        drawList->AddText(
            instructionFont,
            instructionFontSize,
            instructionPosition,
            IM_COL32(235, 245, 255, 255),
            placementInstruction.c_str());
    }

    const std::optional<glm::vec3>& previewPosition =
        mStageAddActorPanel.GetPlacementPreviewPosition();
    if (!previewPosition) {
        return;
    }

    ImVec2 screenPosition;
    if (!mSelectionController.TryWorldToScreenPoint(
            *previewPosition, screenPosition)) {
        return;
    }

    constexpr float markerRadius = 18.0f;
    drawList->AddCircleFilled(
        screenPosition,
        markerRadius,
        IM_COL32(90, 235, 150, 85),
        24);
    drawList->AddCircle(
        screenPosition,
        markerRadius,
        IM_COL32(125, 255, 180, 235),
        24,
        2.5f);
    drawList->AddLine(
        ImVec2(screenPosition.x - markerRadius, screenPosition.y),
        ImVec2(screenPosition.x + markerRadius, screenPosition.y),
        IM_COL32(220, 255, 230, 220),
        1.5f);
    drawList->AddLine(
        ImVec2(screenPosition.x, screenPosition.y - markerRadius),
        ImVec2(screenPosition.x, screenPosition.y + markerRadius),
        IM_COL32(220, 255, 230, 220),
        1.5f);
    const int previewLayer = static_cast<int>(std::round(
        previewPosition->y / mContext.game->GetUGCGridSize()));
    const std::string previewLabel =
        "ここに置く（" + std::to_string(previewLayer + 1) + "だん）";
    drawList->AddText(
        ImVec2(screenPosition.x + 24.0f, screenPosition.y - 8.0f),
        IM_COL32(220, 255, 230, 245),
        previewLabel.c_str());

}

void DebugUIRenderer::ChangeUGCEditLayer(int layerDelta)
{
    constexpr int minimumLayer = 0;
    constexpr int maximumLayer = 20;
    const int nextLayer = std::clamp(
        mUGCEditLayer + layerDelta,
        minimumLayer,
        maximumLayer);
    if (nextLayer == mUGCEditLayer) {
        mUGCStatus = layerDelta < 0
            ? "ここがいちばん下です"
            : "これ以上高くできません";
        return;
    }

    const bool isMovingSelectedActors =
        mIsUGCSelectionDragging &&
        mSelectionController.GetSelectedActorCount() > 0;
    const bool isMovingPlatformDestination =
        isMovingSelectedActors &&
        mIsUGCMovingPlatformDestinationDrag;
    if (isMovingSelectedActors) {
        if (!mHasUGCSelectionDragMoved) {
            mEditCommandController.PushUndo();
            mHasUGCSelectionDragMoved = true;
        }
        const glm::vec3 layerMovement(
            0.0f,
            static_cast<float>(nextLayer - mUGCEditLayer) *
                mContext.game->GetUGCGridSize(),
            0.0f);
        if (isMovingPlatformDestination) {
            const bool wasSaved = mStageAddActorPanel
                .TrySaveUGCMovingPlatformDestinationTranslation(
                    mUGCSelectionDragActorRefs, layerMovement);
            if (!wasSaved) {
                mUGCStatus = "移動先の高さを保存できませんでした";
                return;
            }
            mSelectionController.MoveSelectedMovingPlatformDestinationsByDelta(
                layerMovement);
            mUGCSelectionDragSavedDelta += layerMovement;
        } else {
            mSelectionController.MoveSelectedActorsByDelta(layerMovement);
            mUGCSelectionDragAppliedDelta += layerMovement;
        }

        mUGCSelectionDragPlanePoint += layerMovement;
    }

    mUGCEditLayer = nextLayer;
    mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
    mSelectionController.SetUGCEditLayer(mUGCEditLayer);
    mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
    if (!isMovingSelectedActors) {
        mSelectionController.Clear();
    }
    if (isMovingPlatformDestination) {
        mUGCStatus = "移動先を" + std::to_string(mUGCEditLayer + 1) +
            "だん目へ動かしました";
    } else if (isMovingSelectedActors) {
        mUGCStatus = "選んだものも" + std::to_string(mUGCEditLayer + 1) +
            "だん目へ動かしました";
    } else {
        mUGCStatus = std::to_string(mUGCEditLayer + 1) +
            "だん目を作っています";
    }
}

void DebugUIRenderer::SyncUGCEditLayerToPickedActor()
{
    Actor* pickedActor = mSelectionController.GetPickedActor();
    const std::optional<StageActorRef>& pickedRef =
        mSelectionController.GetPickedActorRef();
    if (!pickedActor || !pickedRef || !mContext.game) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const bool isMovingPlatformDestination =
        mSelectionController.IsMovingPlatformDestinationSelected();
    const glm::vec3 pickedPosition = isMovingPlatformDestination
        ? mSelectionController.CalculateSelectedActorsCenter()
        : pickedActor->GetPos();
    int pickedLayer = static_cast<int>(std::round(
        pickedPosition.y / gridSize));
    YAML::Node stageYaml;
    if (!isMovingPlatformDestination &&
        StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        const YAML::Node sequence = stageYaml[pickedRef->sequenceName];
        if (sequence && sequence.IsSequence() &&
            pickedRef->yamlIndex >= 0 &&
            pickedRef->yamlIndex < static_cast<int>(sequence.size())) {
            const YAML::Node actorNode = sequence[pickedRef->yamlIndex];
            if (actorNode["ugcGeneratedPlatform"] &&
                actorNode["ugcGeneratedPlatform"].as<bool>(false)) {
                pickedLayer = actorNode["ugcGridLayer"].as<int>(pickedLayer);
            }
        }
    }

    constexpr int minimumLayer = 0;
    constexpr int maximumLayer = 20;
    pickedLayer = std::clamp(pickedLayer, minimumLayer, maximumLayer);
    if (pickedLayer == mUGCEditLayer) {
        return;
    }

    mUGCEditLayer = pickedLayer;
    mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
    mSelectionController.SetUGCEditLayer(mUGCEditLayer);
    mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
    mUGCStatus = std::to_string(mUGCEditLayer + 1) +
        "だん目のものを選びました";
}

void DebugUIRenderer::DrawUGCLayerControls()
{
    ImGui::TextUnformatted("いま作っている高さ");
    if (ImGui::Button("▲ ひとつ上のだん", ImVec2(-1.0f, 44.0f))) {
        ChangeUGCEditLayer(1);
    }

    const std::string floorLabel =
        std::to_string(mUGCEditLayer + 1) + " だん目";
    const float labelWidth = ImGui::CalcTextSize(floorLabel.c_str()).x;
    ImGui::SetCursorPosX(std::max(
        ImGui::GetCursorPosX(),
        (ImGui::GetWindowWidth() - labelWidth) * 0.5f));
    ImGui::TextUnformatted(floorLabel.c_str());

    ImGui::BeginDisabled(mUGCEditLayer == 0);
    if (ImGui::Button("▼ ひとつ下のだん", ImVec2(-1.0f, 44.0f))) {
        ChangeUGCEditLayer(-1);
    }
    ImGui::EndDisabled();
    ImGui::TextWrapped(
        "上から見たまま、この高さだけを置いたり消したりできます。");
}

void DebugUIRenderer::DrawUGCTransformControls()
{
    Actor* selectedActor = mSelectionController.GetSingleSelectedActor();
    const std::optional<StageActorRef>& selectedRef =
        mSelectionController.GetPickedActorRef();
    const bool selectedIsConnectionSwitch =
        mUGCConnectionSwitchRef && selectedRef &&
        selectedRef->sequenceName == mUGCConnectionSwitchRef->sequenceName &&
        selectedRef->yamlIndex == mUGCConnectionSwitchRef->yamlIndex;
    if (mUGCConnectionSwitchRef && selectedRef) {
        Platform* targetPlatform = dynamic_cast<Platform*>(selectedActor);
        const bool selectedIsAnotherSwitch = targetPlatform &&
            (targetPlatform->GetPressureSwitchComponent() != nullptr ||
             targetPlatform->GetLatchedGroupSwitchComponent() != nullptr);
        const bool canEditSelectedPlatformConnection =
            !selectedIsConnectionSwitch &&
            selectedRef->sequenceName == "platforms" &&
            targetPlatform != nullptr &&
            !selectedIsAnotherSwitch;
        if (!canEditSelectedPlatformConnection) {
            mUGCStatus =
                "足場を選択してください（惑星やスイッチにはつなげません）";
            mSelectionController.Clear();
            return;
        }

        YAML::Node stageYaml;
        const bool loaded = StageYamlRepository::LoadCurrentStage(
            mContext, stageYaml);
        YAML::Node switchNodes = loaded
            ? stageYaml[mUGCConnectionSwitchRef->sequenceName]
            : YAML::Node();
        YAML::Node targetNodes = loaded
            ? stageYaml[selectedRef->sequenceName]
            : YAML::Node();
        if (switchNodes && targetNodes && switchNodes.IsSequence() &&
            targetNodes.IsSequence() &&
            mUGCConnectionSwitchRef->yamlIndex >= 0 &&
            selectedRef->yamlIndex >= 0 &&
            mUGCConnectionSwitchRef->yamlIndex <
                static_cast<int>(switchNodes.size()) &&
            selectedRef->yamlIndex < static_cast<int>(targetNodes.size())) {
            YAML::Node targetNode = targetNodes[selectedRef->yamlIndex];
            const std::string targetId = targetNode["platformId"]
                ? targetNode["platformId"].as<std::string>() : "";
            const bool wasConnectionChanged =
                mUGCSwitchConnectionAction ==
                    UGCSwitchConnectionAction::Connect
                ? StagePlatformConnections::AssignExclusiveSwitchTarget(
                      stageYaml,
                      mUGCConnectionSwitchRef->yamlIndex,
                      targetId)
                : StagePlatformConnections::DisconnectSwitchTarget(
                      stageYaml,
                      mUGCConnectionSwitchRef->yamlIndex,
                      targetId,
                      selectedRef->yamlIndex);
            if (wasConnectionChanged) {
                StageYamlRepository::SaveCurrentStage(mContext, stageYaml);
                mContext.game->ReloadCurrentStage(StagePhysicsReloadMode::SkipRebuild);
                mUGCStatus = mUGCSwitchConnectionAction ==
                        UGCSwitchConnectionAction::Connect
                    ? "スイッチと足場をつなぎました"
                    : "スイッチと足場のつながりを解除しました";
            } else if (mUGCSwitchConnectionAction ==
                       UGCSwitchConnectionAction::Disconnect) {
                mUGCStatus = "この足場とはつながっていません";
            }
        }
        mUGCConnectionSwitchRef.reset();
        mSelectionController.Clear();
        return;
    }

    if (!selectedActor || !selectedRef || !mContext.gameViewport.IsValid()) {
        return;
    }

    ImVec2 selectedScreenPosition;
    if (!mSelectionController.TryWorldToScreenPoint(
            selectedActor->GetPos(), selectedScreenPosition)) {
        return;
    }

    Platform* selectedPlatform = dynamic_cast<Platform*>(selectedActor);
    const bool isPressureSwitch = selectedPlatform &&
        selectedPlatform->GetPressureSwitchComponent() != nullptr;
    const bool isTwoPlayerSwitch = selectedPlatform &&
        selectedPlatform->GetLatchedGroupSwitchComponent() != nullptr;

    constexpr float buttonWidth = 74.0f;
    constexpr float buttonHeight = 32.0f;
    constexpr float buttonGap = 6.0f;
    constexpr ImGuiWindowFlags actionWindowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoNav;

    if (isPressureSwitch || isTwoPlayerSwitch) {
        const ImVec2 connectPosition(
            selectedScreenPosition.x - buttonWidth,
            selectedScreenPosition.y - buttonHeight - 24.0f);
        ImGui::SetNextWindowPos(connectPosition, ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.82f);
        ImGui::Begin(
            "###UGCSelectedSwitchConnection",
            nullptr,
            actionWindowFlags);
        if (ImGui::Button(
                "つなぐ",
                ImVec2(buttonWidth, buttonHeight))) {
            mIsUGCEraserMode = false;
            mStageAddActorPanel.CancelPlacement();
            mUGCConnectionSwitchRef = *selectedRef;
            mUGCSwitchConnectionAction =
                UGCSwitchConnectionAction::Connect;
            mSelectionController.Clear();
            mUGCStatus = "表示したい足場を1つクリックしてください";
        }
        ImGui::SameLine(0.0f, buttonGap);
        if (ImGui::Button(
                "解除",
                ImVec2(buttonWidth, buttonHeight))) {
            mIsUGCEraserMode = false;
            mStageAddActorPanel.CancelPlacement();
            mUGCConnectionSwitchRef = *selectedRef;
            mUGCSwitchConnectionAction =
                UGCSwitchConnectionAction::Disconnect;
            mSelectionController.Clear();
            mUGCStatus = "つながりを解除したい足場をクリックしてください";
        }
        ImGui::End();
    }
}

void DebugUIRenderer::ToggleUGCVerticalView()
{
    if (!mContext.game || !mContext.game->GetCameraSystem()) {
        return;
    }

    mContext.game->ToggleUGCPreviewVerticalView();
    const bool isViewedFromBelow =
        mContext.game->GetIsUGCPreviewViewedFromBelow();
    const glm::vec3 viewDirection = isViewedFromBelow
        ? glm::vec3(0.0f, -1.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    CameraPose pose = cameraSystem->GetDebugCameraPose();
    const float currentViewDistance = glm::length(
        pose.position - pose.target);
    constexpr float fallbackViewDistance = 30.0f;
    const float viewDistance = currentViewDistance > 0.0001f
        ? currentViewDistance
        : fallbackViewDistance;

    pose.position = pose.target + viewDirection * viewDistance;
    pose.up = isViewedFromBelow
        ? glm::vec3(0.0f, 0.0f, 1.0f)
        : glm::vec3(0.0f, 0.0f, -1.0f);
    cameraSystem->SetDebugCameraPose(pose);

    mUGCViewDirection = viewDirection;
    mContext.game->SetIsUGCOrthographicView(true);
    mContext.game->SetFreeCameraMode(true);
}

void DebugUIRenderer::SetUGCFixedView(const glm::vec3& viewDirection)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() ||
        !mContext.game->GetCurrentStage()) {
        return;
    }

    glm::vec3 stageCenter(0.0f);
    float viewDistance = 24.0f;
    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (!planets.empty()) {
        for (const Planet* planet : planets) {
            if (planet) stageCenter += planet->GetPos();
        }
        stageCenter /= static_cast<float>(planets.size());
        for (const Planet* planet : planets) {
            if (!planet) continue;
            viewDistance = std::max(
                viewDistance,
                glm::length(planet->GetPos() - stageCenter) +
                    std::max({
                        std::abs(planet->GetScale().x),
                        std::abs(planet->GetScale().y),
                        std::abs(planet->GetScale().z)}) * 3.0f);
        }
    }

    const glm::vec3 normalizedDirection = glm::normalize(viewDirection);
    const glm::vec3 absoluteDirection = glm::abs(normalizedDirection);
    const int activeAxisCount =
        (absoluteDirection.x > 0.001f ? 1 : 0) +
        (absoluteDirection.y > 0.001f ? 1 : 0) +
        (absoluteDirection.z > 0.001f ? 1 : 0);
    const bool isAxisAlignedView = activeAxisCount == 1;
    mUGCViewDirection = normalizedDirection;
    CameraPose pose;
    pose.position = stageCenter + normalizedDirection * viewDistance;
    pose.target = stageCenter;
    if (normalizedDirection.y > 0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, -1.0f);
    } else if (normalizedDirection.y < -0.9f) {
        pose.up = glm::vec3(0.0f, 0.0f, 1.0f);
    } else {
        pose.up = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    pose.fieldOfViewDegrees = 55.0f;
    mContext.game->SetIsUGCOrthographicView(isAxisAlignedView);
    if (isAxisAlignedView) {
        const float matchingPerspectiveHalfHeight =
            viewDistance * std::tan(
                glm::radians(pose.fieldOfViewDegrees) * 0.5f);
        mContext.game->SetUGCOrthographicHalfHeight(
            matchingPerspectiveHalfHeight);
    }
    mContext.game->GetCameraSystem()->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
}

void DebugUIRenderer::DrawUGCGridOverlay()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.gameViewport.IsValid()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty() || !planets.front()) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    const glm::vec3 gridCenter(
        std::round(planets.front()->GetPos().x / gridSize) * gridSize,
        static_cast<float>(mUGCEditLayer) * gridSize,
        std::round(planets.front()->GetPos().z / gridSize) * gridSize);

    const CameraPose cameraPose =
        mContext.game->GetCameraSystem()->GetDebugCameraPose();
    const float cameraDistance = glm::length(
        cameraPose.position - gridCenter);
    float requiredGridExtent = std::max(40.0f, cameraDistance * 2.0f);
    for (const Planet* planet : planets) {
        if (!planet) {
            continue;
        }
        requiredGridExtent = std::max(
            requiredGridExtent,
            glm::length(planet->GetPos() - gridCenter) +
                std::max({
                    std::abs(planet->GetScale().x),
                    std::abs(planet->GetScale().y),
                    std::abs(planet->GetScale().z)}) * 2.0f);
    }




    // 配置は常に選択グリッドへスナップする。ズームアウト時だけ表示グリッドを粗くし、極小の線を大量に描かない。
    constexpr int maximumHalfGridLineCount = 120;
    float visualGridSize = gridSize;
    while (requiredGridExtent / visualGridSize >
           static_cast<float>(maximumHalfGridLineCount)) {
        visualGridSize *= 2.0f;
    }
    const int halfGridLineCount = std::max(
        20,
        static_cast<int>(std::ceil(
            requiredGridExtent / visualGridSize)));

    glm::vec3 horizontalAxis(1.0f, 0.0f, 0.0f);
    glm::vec3 verticalAxis(0.0f, 0.0f, 1.0f);
    const glm::vec3 absoluteViewDirection = glm::abs(mUGCViewDirection);
    if (absoluteViewDirection.x > 0.9f) {
        horizontalAxis = glm::vec3(0.0f, 0.0f, 1.0f);
        verticalAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (absoluteViewDirection.z > 0.9f) {
        horizontalAxis = glm::vec3(1.0f, 0.0f, 0.0f);
        verticalAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 clipMin(
        mContext.gameViewport.x,
        mContext.gameViewport.y);
    const ImVec2 clipMax(
        mContext.gameViewport.x + mContext.gameViewport.width,
        mContext.gameViewport.y + mContext.gameViewport.height);
    drawList->PushClipRect(clipMin, clipMax, true);

    ImVec2 screenOrigin;
    ImVec2 screenHorizontalPoint;
    ImVec2 screenVerticalPoint;
    if (!mSelectionController.TryWorldToScreenPoint(
            gridCenter,
            screenOrigin) ||
        !mSelectionController.TryWorldToScreenPoint(
            gridCenter + horizontalAxis * visualGridSize,
            screenHorizontalPoint) ||
        !mSelectionController.TryWorldToScreenPoint(
            gridCenter + verticalAxis * visualGridSize,
            screenVerticalPoint)) {
        drawList->PopClipRect();
        return;
    }

    const ImVec2 horizontalStep(
        screenHorizontalPoint.x - screenOrigin.x,
        screenHorizontalPoint.y - screenOrigin.y);
    const ImVec2 verticalStep(
        screenVerticalPoint.x - screenOrigin.x,
        screenVerticalPoint.y - screenOrigin.y);
    const float horizontalStepLength = std::sqrt(
        horizontalStep.x * horizontalStep.x +
        horizontalStep.y * horizontalStep.y);
    const float verticalStepLength = std::sqrt(
        verticalStep.x * verticalStep.x +
        verticalStep.y * verticalStep.y);
    constexpr float minimumVisibleStepPixels = 2.0f;
    if (horizontalStepLength < minimumVisibleStepPixels ||
        verticalStepLength < minimumVisibleStepPixels) {
        drawList->PopClipRect();
        return;
    }

    const ImVec2 viewportCenter(
        (clipMin.x + clipMax.x) * 0.5f,
        (clipMin.y + clipMax.y) * 0.5f);
    const float viewportDiagonal = std::hypot(
        clipMax.x - clipMin.x,
        clipMax.y - clipMin.y);
    const float originDistance = std::hypot(
        screenOrigin.x - viewportCenter.x,
        screenOrigin.y - viewportCenter.y);
    const int screenHalfLineCount = std::clamp(
        static_cast<int>(std::ceil(
            (viewportDiagonal + originDistance) /
            std::min(horizontalStepLength, verticalStepLength))) + 2,
        halfGridLineCount,
        600);
    const float lineExtension =
        static_cast<float>(screenHalfLineCount) * 2.0f;

    for (int lineIndex = -screenHalfLineCount;
         lineIndex <= screenHalfLineCount;
         ++lineIndex) {
        const float offset = static_cast<float>(lineIndex);
        const bool isCenterLine = lineIndex == 0;
        const ImU32 color = isCenterLine
            ? IM_COL32(125, 205, 255, 115)
            : IM_COL32(210, 225, 235, 42);
        const float thickness = isCenterLine ? 1.5f : 1.0f;

        const ImVec2 horizontalLineOrigin(
            screenOrigin.x + horizontalStep.x * offset,
            screenOrigin.y + horizontalStep.y * offset);
        drawList->AddLine(
            ImVec2(
                horizontalLineOrigin.x - verticalStep.x * lineExtension,
                horizontalLineOrigin.y - verticalStep.y * lineExtension),
            ImVec2(
                horizontalLineOrigin.x + verticalStep.x * lineExtension,
                horizontalLineOrigin.y + verticalStep.y * lineExtension),
            color,
            thickness);

        const ImVec2 verticalLineOrigin(
            screenOrigin.x + verticalStep.x * offset,
            screenOrigin.y + verticalStep.y * offset);
        drawList->AddLine(
            ImVec2(
                verticalLineOrigin.x - horizontalStep.x * lineExtension,
                verticalLineOrigin.y - horizontalStep.y * lineExtension),
            ImVec2(
                verticalLineOrigin.x + horizontalStep.x * lineExtension,
                verticalLineOrigin.y + horizontalStep.y * lineExtension),
            color,
            thickness);
    }
    drawList->PopClipRect();
}

void DebugUIRenderer::DrawUGCStackBadges()
{
    if (!mContext.game || !mContext.gameViewport.IsValid() ||
        std::abs(mUGCViewDirection.y) < 0.9f) {
        return;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
        return;
    }
    const YAML::Node cells = stageYaml["ugcPlatformCells"];
    if (!cells || !cells.IsSequence()) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    std::map<std::pair<int, int>, std::set<int>> columnLayers;
    for (const YAML::Node& cell : cells) {
        const YAML::Node gridPosition = cell["gridPosition"];
        if (!gridPosition || !gridPosition.IsSequence() ||
            gridPosition.size() < 3 || !cell["gridSize"] ||
            std::abs(cell["gridSize"].as<float>(gridSize) - gridSize) >=
                0.0001f) {
            continue;
        }
        columnLayers[{gridPosition[0].as<int>(), gridPosition[2].as<int>()}]
            .insert(gridPosition[1].as<int>());
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    const ImVec2 clipMin(
        mContext.gameViewport.x,
        mContext.gameViewport.y);
    const ImVec2 clipMax(
        mContext.gameViewport.x + mContext.gameViewport.width,
        mContext.gameViewport.y + mContext.gameViewport.height);
    drawList->PushClipRect(clipMin, clipMax, true);
    for (const auto& [cellPosition, layers] : columnLayers) {
        if (layers.size() < 2) {
            continue;
        }
        ImVec2 screenPosition;
        const glm::vec3 worldPosition(
            (static_cast<float>(cellPosition.first) + 0.5f) * gridSize,
            static_cast<float>(mUGCEditLayer) * gridSize,
            (static_cast<float>(cellPosition.second) + 0.5f) * gridSize);
        if (!mSelectionController.TryWorldToScreenPoint(
                worldPosition, screenPosition)) {
            continue;
        }

        const std::string label = "x" + std::to_string(layers.size());
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const float radius = std::max(12.0f, textSize.x * 0.5f + 5.0f);
        drawList->AddCircleFilled(
            screenPosition,
            radius,
            IM_COL32(255, 177, 64, 230),
            20);
        drawList->AddCircle(
            screenPosition,
            radius,
            IM_COL32(255, 244, 210, 255),
            20,
            2.0f);
        drawList->AddText(
            ImVec2(
                screenPosition.x - textSize.x * 0.5f,
                screenPosition.y - textSize.y * 0.5f),
            IM_COL32(35, 27, 18, 255),
            label.c_str());
    }
    drawList->PopClipRect();
}

void DebugUIRenderer::AdjustUGCViewDistance(float distanceMultiplier)
{
    if (!mContext.game || !mContext.game->GetCameraSystem() ||
        !mContext.game->GetCurrentStage() ||
        distanceMultiplier <= 0.0f) {
        return;
    }

    CameraSystem* cameraSystem = mContext.game->GetCameraSystem();
    if (mContext.game->GetIsUGCOrthographicView()) {
        constexpr float minimumHalfHeight = 1.0f;
        constexpr float maximumHalfHeight = 250.0f;
        const float nextHalfHeight = glm::clamp(
            mContext.game->GetUGCOrthographicHalfHeight() *
                distanceMultiplier,
            minimumHalfHeight,
            maximumHalfHeight);
        mContext.game->SetUGCOrthographicHalfHeight(nextHalfHeight);
        return;
    }

    CameraPose pose = cameraSystem->GetDebugCameraPose();

    const glm::vec3 targetToCamera = pose.position - pose.target;
    const float currentDistance = glm::length(targetToCamera);
    constexpr float minimumDistance = 3.0f;
    constexpr float maximumDistance = 250.0f;
    if (currentDistance <= 0.0001f) {
        return;
    }

    const float nextDistance = glm::clamp(
        currentDistance * distanceMultiplier,
        minimumDistance,
        maximumDistance);
    pose.position = pose.target +
        targetToCamera / currentDistance * nextDistance;
    cameraSystem->SetDebugCameraPose(pose);
    mContext.game->SetFreeCameraMode(true);
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
        DrawStarCollectionEditor();
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

void DebugUIRenderer::DrawStarCollectionEditor()
{
    if (!mContext.game) {
        return;
    }

    Star* star = nullptr;
    if (Stage* stage = mContext.game->GetCurrentStage()) {
        for (Planet* planet : stage->GetPlanets()) {
            if (planet && planet->GetStar()) {
                star = planet->GetStar();
                break;
            }
        }
    }
    if (!star) {
        ImGui::TextUnformatted("現在のステージに星がありません。");
        return;
    }

    Star::CollectionAnimationSettings settings = star->GetCollectionAnimationSettings();
    ImGui::TextUnformatted("星獲得演出（カメラは現在のまま）");
    ImGui::DragFloat("周回時間", &settings.orbitDuration, 0.01f, 0.05f, 10.0f, "%.2f 秒");
    ImGui::DragFloat("周回開始半径", &settings.orbitStartRadius, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("周回中の横回転速度", &settings.orbitSpinDegreesPerSecond,
                     5.0f, -3600.0f, 3600.0f, "%.0f 度/秒");
    ImGui::DragFloat("真上の高さ", &settings.finalHeight, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("真上で待つ時間", &settings.waitAbovePlayerDuration, 0.01f, 0.0f, 5.0f, "%.2f 秒");
    ImGui::DragFloat("落下時間", &settings.fallDuration, 0.01f, 0.05f, 10.0f, "%.2f 秒");
    star->SetCollectionAnimationSettings(settings);

    if (ImGui::Button("プレビュー再生")) {
        star->StartCollectionPreview(mContext.game->GetMainPlayer());
    }
    ImGui::SameLine();
    if (ImGui::Button("stars.yamlへ保存")) {
        const bool saved = star->SaveCollectionAnimationSettings();
        mBuildRestartStatus = saved
            ? "星獲得演出を保存しました"
            : "星獲得演出を保存できませんでした";
        mIsBuildRestartStatusError = !saved;
    }
}
