#include "DebugUIRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "actor/Platform.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/session/EditorSessionState.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/CameraSystem.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"


#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <unordered_set>

namespace {
const std::filesystem::path UGCWorkingStagePath =
    "../assets/data/stage/ugc_stage.yaml";
const std::filesystem::path UGCSaveDirectory =
    "../assets/data/stage/ugc_saves";

std::filesystem::path MakeUGCWorkPath(const std::string& fileName)
{
    return UGCSaveDirectory / std::filesystem::u8path(fileName);
}

std::string ToUtf8FileName(const std::filesystem::path& path)
{
    const std::u8string utf8Name = path.filename().u8string();
    return std::string(utf8Name.begin(), utf8Name.end());
}

std::string MakeSafeUGCFileName(const std::string& displayName)
{
    std::string safeName = displayName;
    constexpr const char* invalidCharacters = "<>:\"/\\|?*";
    for (char& character : safeName) {
        const unsigned char unsignedCharacter =
            static_cast<unsigned char>(character);
        if (unsignedCharacter < 32 ||
            std::strchr(invalidCharacters, character)) {
            character = '_';
        }
    }
    while (!safeName.empty() &&
           (safeName.back() == ' ' || safeName.back() == '.')) {
        safeName.pop_back();
    }
    return safeName.empty() ? "untitled" : safeName;
}
}

DebugUIRenderer::DebugUIRenderer(Game* game, UIRenderer* uiRenderer)
    : mContext{game, uiRenderer, &mAssetCatalog},
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
    mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
    mSelectionController.SetUGCEditLayer(mUGCEditLayer);
    if (mContext.game) {
        mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
    }
}

void DebugUIRenderer::HandleUGCUndo()
{
    mUGCStatus = mEditCommandController.RestoreUndo()
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

void DebugUIRenderer::HandleUGCZoom(float distanceMultiplier)
{
    AdjustUGCViewDistance(distanceMultiplier);
}

void DebugUIRenderer::HandleUGCLayerChange(int layerDelta)
{
    ChangeUGCEditLayer(layerDelta);
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
    mSelectionController.MoveSelectedActorsByDelta(movement);
    if (mStageAddActorPanel.TryTranslateUGCPlatformCells(
            selectedRefs, movement)) {
        mSelectionController.Clear();
    }
    mUGCStatus = "選んだものを1マス動かしました";
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

    // Older projects stored each toolbar as one invisible panel.  Keep those
    // panels as migration anchors, but create editable entries for every
    // actual button so a click selects the eraser/undo/etc. individually.
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
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
        ImGui::PopID();
        return clicked;
    };

    struct PresetButton {
        const char* label;
        const char* modelPath;
        UGCPresetKind kind;
    };
    constexpr std::array<PresetButton, 9> presetButtons = {{
        {"足場", "platform.obj", UGCPresetKind::NormalPlatform},
        {"敵", "enemy.obj", UGCPresetKind::NormalEnemy},
        {"惑星", "planet.obj", UGCPresetKind::EllipsePlanet},
        {"スイッチ", "platform.obj", UGCPresetKind::PressureSwitch},
        {"ゴール", "star.obj", UGCPresetKind::GoalStar},
        {"移動足場", "platform.obj", UGCPresetKind::MovingPlatform},
        {"消える足場", "platform.obj", UGCPresetKind::FadingPlatform},
        {"くっつき足場", "platform.obj", UGCPresetKind::AdhesivePlatform},
        {"2人用スイッチ", "platform.obj", UGCPresetKind::TwoPlayerSwitch},
    }};
    if (mUGCModelThumbnailRenderer) {
        mUGCModelThumbnailRenderer->BeginFrame();
    }
    constexpr float presetIconSize = 52.0f;
    constexpr float presetSlotWidth = 62.0f;
    constexpr std::array<const char*, 9> presetIds = {{
        "presetPlatform", "presetEnemy", "presetPlanet", "presetSwitch", "presetGoal",
        "presetMoving", "presetFading", "presetAdhesive", "presetTwoPlayer",
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
        const GLuint thumbnail = mUGCModelThumbnailRenderer
            ? mUGCModelThumbnailRenderer->ResolveThumbnail(preset.modelPath)
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
            ImGui::OpenPopup("作品管理###UGCWorkManagement");
        }
        if (ImGui::MenuItem("完成チェック")) {
            YAML::Node stageYaml;
            const bool loaded =
                StageYamlRepository::LoadCurrentStage(mContext, stageYaml);
            const YAML::Node stars =
                loaded ? stageYaml["star"] : YAML::Node();
            if (!stars || !stars.IsSequence() || stars.size() == 0) {
                mUGCStatus = "完成チェックにはゴールを置いてください";
            } else if (!SaveCurrentUGCWork(mUGCWorkName.data())) {
                mUGCStatus = "下書きを保存できませんでした: " +
                    mUGCWorkSaveError;
            } else {
                mContext.game->StartUGCClearVerification(
                    MakeSafeUGCFileName(mUGCWorkName.data()) + ".yaml");
            }
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

    // Each button lives in its own ImGui window.  Draw those windows in
    // z-order so a larger value is actually in front when buttons overlap.
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
        {"previewView", "textures/ugc_ui/editor_action_zoom_out.png",
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
            mContext.game->ToggleUGCPreviewVerticalView();
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

    DrawUGCGridOverlay();
    DrawUGCStackBadges();
    DrawUGCPlacementPreview();
    DrawUGCPreviewOverlay();
    DrawUGCWorkManagement();
}

void DebugUIRenderer::RefreshUGCWorkList()
{
    mUGCWorkFileNames.clear();
    std::error_code fileSystemError;
    std::filesystem::create_directories(
        UGCSaveDirectory, fileSystemError);
    if (fileSystemError) {
        mSelectedUGCWorkIndex = -1;
        return;
    }

    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(
             UGCSaveDirectory, fileSystemError)) {
        if (fileSystemError) {
            break;
        }
        if (entry.is_regular_file() &&
            entry.path().extension() == ".yaml") {
            mUGCWorkFileNames.emplace_back(
                ToUtf8FileName(entry.path()));
        }
    }
    std::sort(mUGCWorkFileNames.begin(), mUGCWorkFileNames.end());
    if (mUGCWorkFileNames.empty()) {
        mSelectedUGCWorkIndex = -1;
    } else {
        mSelectedUGCWorkIndex = glm::clamp(
            mSelectedUGCWorkIndex,
            0,
            static_cast<int>(mUGCWorkFileNames.size()) - 1);
    }
    mHasLoadedUGCWorkList = true;
}

bool DebugUIRenderer::SaveCurrentUGCWork(
    const std::string& displayName)
{
    // UGC placement writes its grid cells and objects to the working YAML as
    // soon as they are changed. Do not run the general-purpose editor
    // transform saver here: it walks all runtime actors (including rebuilt
    // UGC platform regions) even though none of that work is needed to make
    // a copy of a UGC project.
    mUGCWorkSaveError.clear();
    try {
        const std::string fileName =
            MakeSafeUGCFileName(displayName) + ".yaml";
        YAML::Node stageYaml;
        if (!StageYamlRepository::LoadCurrentStage(
                mContext, stageYaml)) {
            mUGCWorkSaveError = "作業中のステージを読み込めませんでした";
            return false;
        }
        stageYaml["ugcMetadata"]["displayName"] = displayName;
        if (!stageYaml["ugcMetadata"]["isClearVerified"]) {
            stageYaml["ugcMetadata"]["isClearVerified"] = false;
        }
        if (!StageYamlRepository::SaveCurrentStage(
                mContext, stageYaml)) {
            mUGCWorkSaveError = "作業中のステージを書き込めませんでした";
            return false;
        }
        std::error_code fileSystemError;
        std::filesystem::create_directories(
            UGCSaveDirectory, fileSystemError);
        if (fileSystemError) {
            mUGCWorkSaveError =
                "保存フォルダを作れませんでした: " +
                fileSystemError.message();
            return false;
        }
        std::filesystem::copy_file(
            UGCWorkingStagePath,
            MakeUGCWorkPath(fileName),
            std::filesystem::copy_options::overwrite_existing,
            fileSystemError);
        if (fileSystemError) {
            mUGCWorkSaveError =
                "作品ファイルをコピーできませんでした: " +
                fileSystemError.message();
            return false;
        }

        // Refreshing a vector which is also shown by the open popup in the
        // same ImGui frame can invalidate its active list state. Refresh it
        // on the next frame instead.
        mShouldRefreshUGCWorkList = true;
        return true;
    } catch (const std::exception& error) {
        std::cerr << "Failed to save UGC work: " << error.what() << std::endl;
        mUGCWorkSaveError = error.what();
        return false;
    }
}

bool DebugUIRenderer::IsUGCWorkClearVerified(
    const std::string& workFileName) const
{
    try {
        const YAML::Node stageYaml = YAML::LoadFile(
            MakeUGCWorkPath(workFileName).string());
        return stageYaml["ugcMetadata"] &&
            stageYaml["ugcMetadata"]["isClearVerified"].as<bool>(false);
    } catch (const YAML::Exception&) {
        return false;
    }
}

bool DebugUIRenderer::CompleteUGCVerification(
    const std::string& workFileName)
{
    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext, stageYaml)) {
        return false;
    }
    stageYaml["ugcMetadata"]["isClearVerified"] = true;
    if (!StageYamlRepository::SaveCurrentStage(
            mContext, stageYaml)) {
        return false;
    }

    std::error_code fileSystemError;
    std::filesystem::create_directories(
        UGCSaveDirectory, fileSystemError);
    if (fileSystemError) {
        return false;
    }
    std::filesystem::copy_file(
        UGCWorkingStagePath,
        MakeUGCWorkPath(workFileName),
        std::filesystem::copy_options::overwrite_existing,
        fileSystemError);
    RefreshUGCWorkList();
    return !fileSystemError;
}

bool DebugUIRenderer::LoadSelectedUGCWork()
{
    if (!CopySelectedUGCWorkToWorkingFile()) {
        return false;
    }
    mSelectionController.Clear();
    mContext.game->ReloadCurrentStage();
    return true;
}

bool DebugUIRenderer::CopySelectedUGCWorkToWorkingFile()
{
    if (mSelectedUGCWorkIndex < 0 ||
        mSelectedUGCWorkIndex >=
            static_cast<int>(mUGCWorkFileNames.size())) {
        return false;
    }
    std::error_code fileSystemError;
    std::filesystem::copy_file(
        MakeUGCWorkPath(
            mUGCWorkFileNames[mSelectedUGCWorkIndex]),
        UGCWorkingStagePath,
        std::filesystem::copy_options::overwrite_existing,
        fileSystemError);
    if (fileSystemError) {
        return false;
    }
    return true;
}

bool DebugUIRenderer::DuplicateSelectedUGCWork()
{
    if (mSelectedUGCWorkIndex < 0 ||
        mSelectedUGCWorkIndex >=
            static_cast<int>(mUGCWorkFileNames.size())) {
        return false;
    }
    const std::filesystem::path sourceFilePath = MakeUGCWorkPath(
        mUGCWorkFileNames[mSelectedUGCWorkIndex]);
    const std::u8string sourceStemUtf8 = sourceFilePath.stem().u8string();
    const std::string sourceStem(
        sourceStemUtf8.begin(), sourceStemUtf8.end());
    std::string destinationFileName = sourceStem + "_コピー.yaml";
    int suffix = 2;
    while (std::filesystem::exists(
        MakeUGCWorkPath(destinationFileName))) {
        destinationFileName = sourceStem +
            "_コピー" + std::to_string(suffix++) + ".yaml";
    }
    std::error_code fileSystemError;
    std::filesystem::copy_file(
        sourceFilePath,
        MakeUGCWorkPath(destinationFileName),
        std::filesystem::copy_options::none,
        fileSystemError);
    if (fileSystemError) {
        return false;
    }
    RefreshUGCWorkList();
    return true;
}

bool DebugUIRenderer::DeleteSelectedUGCWork()
{
    if (mSelectedUGCWorkIndex < 0 ||
        mSelectedUGCWorkIndex >=
            static_cast<int>(mUGCWorkFileNames.size())) {
        return false;
    }
    std::error_code fileSystemError;
    const bool removed = std::filesystem::remove(
        MakeUGCWorkPath(
            mUGCWorkFileNames[mSelectedUGCWorkIndex]),
        fileSystemError);
    RefreshUGCWorkList();
    return removed && !fileSystemError;
}

void DebugUIRenderer::DrawUGCWorkManagement()
{
    if (!ImGui::BeginPopupModal(
            "作品管理###UGCWorkManagement",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    if (!mHasLoadedUGCWorkList || mShouldRefreshUGCWorkList) {
        RefreshUGCWorkList();
        mShouldRefreshUGCWorkList = false;
    }

    ImGui::TextUnformatted("現在のステージを作品として保存");
    ImGui::SetNextItemWidth(320.0f);
    ImGui::InputText(
        "作品名###UGCWorkName",
        mUGCWorkName.data(),
        mUGCWorkName.size());
    if (ImGui::Button("保存", ImVec2(120.0f, 0.0f))) {
        if (SaveCurrentUGCWork(mUGCWorkName.data())) {
            mUGCStatus = "作品を保存しました";
        } else {
            mUGCStatus = "作品を保存できませんでした: " +
                mUGCWorkSaveError;
        }
    }

    ImGui::TextUnformatted("保存した作品");
    if (ImGui::BeginListBox(
            "###UGCWorkList", ImVec2(440.0f, 180.0f))) {
        for (int index = 0;
             index < static_cast<int>(mUGCWorkFileNames.size());
             ++index) {
            const std::string displayName = ToUtf8FileName(
                MakeUGCWorkPath(mUGCWorkFileNames[index]).stem());
            if (ImGui::Selectable(
                    displayName.c_str(),
                    mSelectedUGCWorkIndex == index)) {
                mSelectedUGCWorkIndex = index;
            }
        }
        ImGui::EndListBox();
    }
    if (ImGui::Button("開く")) {
        mUGCStatus = LoadSelectedUGCWork()
            ? "作品を開きました"
            : "作品を開けませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("作品を複製")) {
        mUGCStatus = DuplicateSelectedUGCWork()
            ? "作品を複製しました"
            : "作品を複製できませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("作品を削除")) {
        mUGCStatus = DeleteSelectedUGCWork()
            ? "作品を削除しました"
            : "作品を削除できませんでした";
    }
    ImGui::SameLine();
    if (ImGui::Button("閉じる")) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void DebugUIRenderer::DrawUGCWorkBrowser()
{
    if (!mHasLoadedUGCWorkList) {
        RefreshUGCWorkList();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 panelSize(
        std::min(760.0f, viewport->WorkSize.x - 48.0f),
        std::min(620.0f, viewport->WorkSize.y - 48.0f));
    ImGui::SetNextWindowPos(
        ImVec2(
            viewport->WorkPos.x +
                (viewport->WorkSize.x - panelSize.x) * 0.5f,
            viewport->WorkPos.y +
                (viewport->WorkSize.y - panelSize.y) * 0.5f),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(panelSize, ImGuiCond_Always);
    constexpr ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin(
        "つくったステージ###UGCWorkBrowser",
        nullptr,
        windowFlags);
    ImGui::TextUnformatted("つくったステージ");
    ImGui::TextDisabled(
        "作品を選んで、遊ぶか続きを作るか選んでください。");
    ImGui::Separator();

    const float listHeight = std::max(180.0f, panelSize.y - 190.0f);
    int verifiedWorkCount = 0;
    if (ImGui::BeginListBox(
            "###UGCBrowserWorkList",
            ImVec2(-1.0f, listHeight))) {
        for (int index = 0;
             index < static_cast<int>(mUGCWorkFileNames.size());
             ++index) {
            const std::string& fileName = mUGCWorkFileNames[index];
            if (!IsUGCWorkClearVerified(fileName)) {
                continue;
            }
            ++verifiedWorkCount;
            const std::string displayName = ToUtf8FileName(
                MakeUGCWorkPath(fileName).stem());
            if (ImGui::Selectable(
                    displayName.c_str(),
                    mSelectedUGCWorkIndex == index,
                    0,
                    ImVec2(0.0f, 40.0f))) {
                mSelectedUGCWorkIndex = index;
            }
        }
        if (verifiedWorkCount == 0) {
            ImGui::TextDisabled(
                "クリア確認済みの作品はまだありません");
        }
        ImGui::EndListBox();
    }

    const bool hasSelectedWork =
        mSelectedUGCWorkIndex >= 0 &&
        mSelectedUGCWorkIndex <
            static_cast<int>(mUGCWorkFileNames.size()) &&
        IsUGCWorkClearVerified(
            mUGCWorkFileNames[mSelectedUGCWorkIndex]);
    ImGui::BeginDisabled(!hasSelectedWork);
    if (ImGui::Button("あそぶ", ImVec2(150.0f, 44.0f))) {
        if (CopySelectedUGCWorkToWorkingFile() &&
            mContext.game->StartUGCMode()) {
            mContext.game->StartUGCPlaytest();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("つづきから作る", ImVec2(180.0f, 44.0f))) {
        if (CopySelectedUGCWorkToWorkingFile()) {
            mContext.game->StartUGCMode();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("複製", ImVec2(100.0f, 44.0f))) {
        DuplicateSelectedUGCWork();
    }
    ImGui::SameLine();
    if (ImGui::Button("削除", ImVec2(100.0f, 44.0f))) {
        ImGui::OpenPopup("作品を削除しますか###UGCDeleteConfirmation");
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("タイトルへ戻る", ImVec2(150.0f, 44.0f)) ||
        ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        mContext.game->CloseUGCWorkBrowser();
    }

    if (ImGui::BeginPopupModal(
            "作品を削除しますか###UGCDeleteConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted(
            "選んだ作品を削除します。この操作は元に戻せません。");
        if (ImGui::Button("削除する")) {
            DeleteSelectedUGCWork();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("やめる")) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

void DebugUIRenderer::DrawUGCEditor(
    unsigned int gameViewTexture,
    int gameViewWidth,
    int gameViewHeight)
{
    const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    constexpr float topBarHeight = 104.0f;
    const ImVec2 gameViewportMin(
        mainViewport->WorkPos.x,
        mainViewport->WorkPos.y);
    const ImVec2 gameViewportMax(
        mainViewport->WorkPos.x + mainViewport->WorkSize.x,
        mainViewport->WorkPos.y + mainViewport->WorkSize.y);
    const auto resolveUGCAnchor = [&](const char* id, const ImVec2& fallback) {
        if (!mContext.uiRenderer || !mContext.uiRenderer->GetUILoadSystem()) {
            return fallback;
        }
        for (const UILoadSystem::CustomElement& element :
             mContext.uiRenderer->GetUILoadSystem()->GetCustomElements()) {
            if (element.screen == "ugc" && element.id == id) {
                return ImVec2(
                    mainViewport->WorkPos.x +
                        mainViewport->WorkSize.x * element.xRatio,
                    mainViewport->WorkPos.y +
                        mainViewport->WorkSize.x * element.yRatio);
            }
        }
        return fallback;
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
        ImGuiWindowFlags_NoScrollbar;

    ImGui::SetNextWindowPos(
        resolveUGCAnchor("presetTools", mainViewport->WorkPos),
        ImGuiCond_Always);
    constexpr float presetToolbarWidth = 720.0f;
    ImGui::SetNextWindowSize(
        ImVec2(presetToolbarWidth, topBarHeight),
        ImGuiCond_Always);
    ImGui::Begin("###UGCTopBar", nullptr, fixedWindowFlags);
    struct PresetButton {
        const char* label;
        const char* modelPath;
        UGCPresetKind kind;
    };
    constexpr std::array<PresetButton, 9> presetButtons = {{
        {"足場", "platform.obj", UGCPresetKind::NormalPlatform},
        {"敵", "enemy.obj", UGCPresetKind::NormalEnemy},
        {"惑星", "planet.obj", UGCPresetKind::EllipsePlanet},
        {"スイッチ", "platform.obj", UGCPresetKind::PressureSwitch},
        {"ゴール", "star.obj", UGCPresetKind::GoalStar},
        {"移動足場", "platform.obj", UGCPresetKind::MovingPlatform},
        {"消える足場", "platform.obj", UGCPresetKind::FadingPlatform},
        {"くっつき足場", "platform.obj", UGCPresetKind::AdhesivePlatform},
        {"2人用スイッチ", "platform.obj", UGCPresetKind::TwoPlayerSwitch},
    }};
    constexpr float itemButtonWidth = 80.0f;
    ImGui::SetCursorPosX(std::max(
        0.0f,
        (presetToolbarWidth -
         itemButtonWidth * presetButtons.size()) * 0.5f));
    for (const PresetButton& preset : presetButtons) {
        ImGui::PushID(preset.label);
        const GLuint thumbnail = mUGCModelThumbnailRenderer
            ? mUGCModelThumbnailRenderer->ResolveThumbnail(preset.modelPath)
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
                  ImVec2(60.0f, 60.0f),
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(preset.label, ImVec2(60.0f, 60.0f));
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
        ImGui::SameLine();
    }
    ImGui::NewLine();
    const bool usesUGCPlatformFootprint =
        mActiveUGCPresetKind == UGCPresetKind::NormalPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::MovingPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::FadingPlatform ||
        mActiveUGCPresetKind == UGCPresetKind::AdhesivePlatform;
    if (usesUGCPlatformFootprint) {
        ImGui::SetCursorPosX((presetToolbarWidth - 220.0f) * 0.5f);
        ImGui::TextUnformatted("大きさ"); ImGui::SameLine();
        const int sizes[] = {1, 2, 3}; const char* labels[] = {"1マス", "4マス", "9マス"};
        for (int index = 0; index < 3; ++index) {
            if (ImGui::Selectable(labels[index], mUGCPlatformFootprintSideLength == sizes[index], 0, ImVec2(52.0f, 0.0f))) {
                mUGCPlatformFootprintSideLength = sizes[index];
                mStageAddActorPanel.SetUGCPlatformFootprintSideLength(sizes[index]);
            }
            ImGui::SameLine();
        }
    }
    DrawUGCWorkManagement();
    ImGui::End();

    constexpr ImGuiWindowFlags floatingButtonFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;
    const auto drawActionIcon = [&](const char* id, const char* texturePath, const char* tooltip) {
        ImGui::PushID(id);
        const bool hasTexture = mContext.uiRenderer &&
            mContext.uiRenderer->RegisterCustomUITexture(texturePath);
        const GLuint texture = hasTexture
            ? mContext.uiRenderer->GetCustomUITextureHandle(texturePath)
            : 0;
        // The icon artwork already contains its blue button.  Keep Dear ImGui's
        // own frame fully transparent so it does not add a second blue backing.
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.18f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.32f));
        const bool clicked = texture != 0
            ? ImGui::ImageButton(
                  "##actionIcon",
                  static_cast<ImTextureID>(texture),
                  ImVec2(64.0f, 64.0f),
                  ImVec2(0.0f, 1.0f),
                  ImVec2(1.0f, 0.0f))
            : ImGui::Button(tooltip, ImVec2(64.0f, 64.0f));
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
        return clicked;
    };
    ImGui::SetNextWindowPos(
        resolveUGCAnchor("quickTools", ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 108.0f)),
        ImGuiCond_Always);
    ImGui::Begin("###UGCQuickTools", nullptr, floatingButtonFlags);
    if (drawActionIcon("eraser", "textures/ugc_ui/editor_action_eraser.png", mIsUGCEraserMode ? "消しゴム：ON" : "消しゴム")) {
        HandleUGCEraserToggle();
    }
    if (drawActionIcon("undo", "textures/ugc_ui/editor_action_undo.png", "1つ戻す")) HandleUGCUndo();
    if (drawActionIcon("redo", "textures/ugc_ui/editor_action_redo.png", "やり直す")) HandleUGCRedo();
    ImGui::End();

    // Controller users have button shortcuts; keyboard and mouse users need
    // the equivalent editor actions on screen as well.
    ImGui::SetNextWindowPos(
        resolveUGCAnchor("keyboardTools", ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 16.0f)),
        ImGuiCond_Always);
    ImGui::Begin("###UGCKeyboardTools", nullptr, floatingButtonFlags);
    if (drawActionIcon("layerUp", "textures/ugc_ui/editor_action_layer_up.png", "上のだん")) {
        HandleUGCLayerChange(1);
    }
    if (drawActionIcon("layerDown", "textures/ugc_ui/editor_action_layer_down.png", "下のだん")) {
        HandleUGCLayerChange(-1);
    }
    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    if (drawActionIcon("zoomIn", "textures/ugc_ui/editor_action_zoom_in.png", "近づく")) {
        HandleUGCZoom(0.85f);
    }
    if (drawActionIcon("zoomOut", "textures/ugc_ui/editor_action_zoom_out.png", "遠ざかる")) {
        HandleUGCZoom(1.18f);
    }
    if (drawActionIcon(
            "previewView",
            "textures/ugc_ui/editor_action_zoom_out.png",
            mContext.game->GetIsUGCPreviewViewedFromBelow()
                ? "上から見る"
                : "下から見る")) {
        mContext.game->ToggleUGCPreviewVerticalView();
    }
    ImGui::End();

    ImGui::SetNextWindowPos(
        resolveUGCAnchor("play", ImVec2(gameViewportMin.x + 16.0f, gameViewportMax.y - 74.0f)),
        ImGuiCond_Always);
    ImGui::Begin("###UGCPlay", nullptr, floatingButtonFlags);
    if (drawActionIcon("play", "textures/ugc_ui/editor_action_play.png", "遊ぶ")) {
        mContext.game->StartUGCPlaytest();
        ImGui::End();
        return;
    }
    ImGui::End();

    ImGui::SetNextWindowPos(
        // Keep the menu with the edit actions: it is the first button above
        // eraser, undo and redo instead of floating beside the 3D preview.
        resolveUGCAnchor("menu", ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 32.0f)),
        ImGuiCond_Always);
    ImGui::Begin("###UGCMenu", nullptr, floatingButtonFlags);
    if (drawActionIcon("menu", "textures/ugc_ui/editor_action_menu.png", "メニュー")) {
        ImGui::OpenPopup("メニュー###UGCProductMenu");
    }
    if (ImGui::BeginPopup("メニュー###UGCProductMenu")) {
        if (ImGui::MenuItem("保存・開く")) {
            ImGui::OpenPopup("作品管理###UGCWorkManagement");
        }
        if (ImGui::MenuItem("完成チェック")) {
            YAML::Node stageYaml;
            const bool loaded = StageYamlRepository::LoadCurrentStage(mContext, stageYaml);
            const YAML::Node stars = loaded ? stageYaml["star"] : YAML::Node();
            if (!stars || !stars.IsSequence() || stars.size() == 0) {
                mUGCStatus = "完成チェックにはゴールを置いてください";
            } else if (!SaveCurrentUGCWork(mUGCWorkName.data())) {
                mUGCStatus = "下書きを保存できませんでした: " + mUGCWorkSaveError;
            } else {
                mContext.game->StartUGCClearVerification(
                    MakeSafeUGCFileName(mUGCWorkName.data()) + ".yaml");
            }
        }
        if (ImGui::MenuItem("タイトルへ戻る")) {
            mContext.game->ExitUGCMode();
        }
        if (!mSelectionController.GetSelectedKeys().empty()) {
            ImGui::Separator();
            DrawUGCTransformControls();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
    DrawUGCViewport(
        gameViewTexture,
        gameViewWidth,
        gameViewHeight,
        gameViewportMin,
        gameViewportMax);
    DrawUGCGridOverlay();
    DrawUGCStackBadges();
    DrawUGCPlacementPreview();
    DrawUGCPreviewOverlay();

    if (mStageAddActorPanel.IsPlacementActive()) {
        mStageAddActorPanel.UpdatePlacement();
    } else {
        mSelectionController.Update();
        if (!mIsUGCEraserMode &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            SyncUGCEditLayerToPickedActor();
        }
        if (!mIsUGCEraserMode) {
            UpdateUGCSelectionDrag();
        }
        if (mIsUGCEraserMode &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
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

                // Generated UGC platforms are erased cell-by-cell below.
                // All other placed actors (enemy, star, switches, etc.) take
                // priority when they belong to the currently edited layer.
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

            if (tryDeletePickedActorOnCurrentLayer()) {
                mUGCStatus = "今のだんのものを消しました";
            } else if (mStageAddActorPanel.TryEraseUGCPlatformCell()) {
                mUGCStatus = "足場を1マス消しました";
            } else if (tryDeletePickedActorAsHighestFallback()) {
                mUGCStatus = "いちばん上のものを消しました";
            } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                       !mSelectionController.GetSelectedKeys().empty()) {
                const std::optional<StageActorRef>& selectedRef =
                    mSelectionController.GetPickedActorRef();
                if (selectedRef &&
                    selectedRef->type == StageActorType::Planet) {
                    mPendingUGCPlanetDeleteIndex = selectedRef->yamlIndex;
                    ImGui::OpenPopup(
                        "惑星を消しますか###UGCPlanetDeleteConfirmation");
                } else {
                    const std::unordered_set<std::string> selectedKeys =
                        mSelectionController.GetSelectedKeys();
                    if (mEditCommandController.DeleteSelectedKeys(selectedKeys)) {
                        mUGCStatus = "選んだものを消しました";
                    }
                }
            }
        }
    }
    DrawUGCPlanetDeleteConfirmation();
    mSelectionController.ApplyEditorSelectionFlags();
    if (mContext.uiRenderer) {
        mContext.uiRenderer->DrawUGCForegroundCustomUI(
            gameViewportMin,
            ImVec2(
                gameViewportMax.x - gameViewportMin.x,
                gameViewportMax.y - gameViewportMin.y));
    }
}

void DebugUIRenderer::DrawUGCPlanetDeleteConfirmation()
{
    if (!ImGui::BeginPopupModal(
            "惑星を消しますか###UGCPlanetDeleteConfirmation",
            nullptr,
            ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::TextWrapped(
        "この惑星と、惑星の上に置いたものをぜんぶ消します。");
    ImGui::TextUnformatted("本当に消しますか？");
    if (ImGui::Button("消す", ImVec2(120.0f, 40.0f))) {
        const bool deleted = mPendingUGCPlanetDeleteIndex &&
            mEditCommandController.DeletePlanet(
                *mPendingUGCPlanetDeleteIndex);
        mUGCStatus = deleted
            ? "惑星を消しました"
            : "最後の惑星は消せません";
        mPendingUGCPlanetDeleteIndex.reset();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("やめる", ImVec2(120.0f, 40.0f))) {
        mPendingUGCPlanetDeleteIndex.reset();
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
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
        mHasUGCSelectionDragMoved = false;
        mUGCSelectionDragOffset =
            selectionCenter - intersection;
        mUGCSelectionDragInitialCenter = selectionCenter;
        mUGCSelectionDragActorRefs.clear();
        mUGCSelectionDragActorRefs.reserve(selectedInstances.size());
        for (const StageActorInstance& selectedInstance : selectedInstances) {
            mUGCSelectionDragActorRefs.emplace_back(selectedInstance.ref);
        }
        return;
    }

    if (!isMouseDown) {
        if (mHasUGCSelectionDragMoved) {
            const glm::vec3 finalCenter =
                mSelectionController.CalculateSelectedActorsCenter();
            const glm::vec3 totalDelta =
                finalCenter - mUGCSelectionDragInitialCenter;
            mStagePlacementPanel.SaveEditorAuthoredTransforms();
            if (mStageAddActorPanel.TryTranslateUGCPlatformCells(
                    mUGCSelectionDragActorRefs, totalDelta)) {
                mSelectionController.Clear();
            }
            mUGCStatus = "移動しました";
        }
        mIsUGCSelectionDragging = false;
        mHasUGCSelectionDragMoved = false;
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
    const glm::vec3 currentCenter =
        mSelectionController.CalculateSelectedActorsCenter();
    const glm::vec3 movementDelta = snappedTarget - currentCenter;
    if (glm::length(movementDelta) <= 0.000001f) {
        return;
    }

    if (!mHasUGCSelectionDragMoved) {
        mEditCommandController.PushUndo();
        mHasUGCSelectionDragMoved = true;
    }
    mSelectionController.MoveSelectedActorsByDelta(movementDelta);
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
        // Start large enough to inspect the stage, while keeping room for a
        // user to enlarge it up to the half-screen limit.
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
        "3Dプレビュー");
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
            previewElement->widthRatio =
                mUGCPreviewWidth /
                std::max(mContext.gameViewport.width, 1.0f);
            previewElement->heightRatio =
                previewElement->widthRatio * 9.0f / 16.0f;
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

    const auto projectPoint = [&](const glm::vec3& worldPoint,
                                  ImVec2& outScreenPoint) {
        const glm::vec4 clip = projection * view * glm::vec4(worldPoint, 1.0f);
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
    const auto drawWorldLine = [&](const glm::vec3& from,
                                   const glm::vec3& to,
                                   ImU32 color,
                                   float thickness) {
        ImVec2 screenFrom;
        ImVec2 screenTo;
        if (projectPoint(from, screenFrom) && projectPoint(to, screenTo)) {
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
        // Keep height labels on the preview edge.  They remain a readable
        // height ruler without ever being drawn over a platform.
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
    const std::optional<glm::vec3>& previewPosition =
        mStageAddActorPanel.GetPlacementPreviewPosition();
    if (!previewPosition || !mContext.gameViewport.IsValid()) {
        return;
    }

    ImVec2 screenPosition;
    if (!mSelectionController.TryWorldToScreenPoint(
            *previewPosition, screenPosition)) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
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
        mSelectionController.MoveSelectedActorsByDelta(layerMovement);
        // The drag plane follows the selection vertically. This keeps the
        // held object on its newly selected layer while horizontal dragging
        // continues, and preserves the whole move as one undo operation.
        mUGCSelectionDragPlanePoint += layerMovement;
    }

    mUGCEditLayer = nextLayer;
    mStageAddActorPanel.SetUGCEditLayer(mUGCEditLayer);
    mSelectionController.SetUGCEditLayer(mUGCEditLayer);
    mContext.game->SetUGCPreviewEditLayer(mUGCEditLayer);
    if (!isMovingSelectedActors) {
        mSelectionController.Clear();
    }
    mUGCStatus = isMovingSelectedActors
        ? "選んだものも" + std::to_string(mUGCEditLayer + 1) +
              "だん目へ動かしました"
        : std::to_string(mUGCEditLayer + 1) +
              "だん目を作っています";
}

void DebugUIRenderer::SyncUGCEditLayerToPickedActor()
{
    Actor* pickedActor = mSelectionController.GetPickedActor();
    const std::optional<StageActorRef>& pickedRef =
        mSelectionController.GetPickedActorRef();
    if (!pickedActor || !pickedRef ||
        dynamic_cast<Planet*>(pickedActor) != nullptr ||
        !mContext.game) {
        return;
    }

    const float gridSize = mContext.game->GetUGCGridSize();
    int pickedLayer = static_cast<int>(std::round(
        pickedActor->GetPos().y / gridSize));
    YAML::Node stageYaml;
    if (StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
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
    if (!selectedActor) {
        ImGui::TextWrapped(
            "ゲーム画面のものを1つ選ぶと、高さや向きを変えられます。");
        return;
    }

    bool isGeneratedUGCPlatform = false;
    const std::optional<StageActorRef>& selectedRef =
        mSelectionController.GetPickedActorRef();

    const bool selectedIsConnectionSwitch =
        mUGCConnectionSwitchRef && selectedRef &&
        selectedRef->sequenceName == mUGCConnectionSwitchRef->sequenceName &&
        selectedRef->yamlIndex == mUGCConnectionSwitchRef->yamlIndex;
    if (mUGCConnectionSwitchRef && selectedRef &&
        !selectedIsConnectionSwitch &&
        selectedRef->sequenceName == "platforms") {
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
            YAML::Node targets = switchNodes[
                mUGCConnectionSwitchRef->yamlIndex]["components"]
                ["pressureSwitch"]["targets"];
            bool alreadyConnected = targetId.empty();
            for (const YAML::Node& target : targets) {
                alreadyConnected = alreadyConnected ||
                    (target.as<std::string>("") == targetId);
            }
            if (!alreadyConnected) {
                targets.push_back(targetId);
                StageYamlRepository::SaveCurrentStage(mContext, stageYaml);
                mContext.game->ReloadCurrentStage(false);
                mUGCStatus = "スイッチと足場をつなぎました";
            }
        }
        mUGCConnectionSwitchRef.reset();
        mSelectionController.Clear();
        return;
    }
    if (selectedRef) {
        YAML::Node stageYaml;
        if (StageYamlRepository::LoadCurrentStage(mContext, stageYaml)) {
            const YAML::Node sequence = stageYaml[selectedRef->sequenceName];
            if (sequence && sequence.IsSequence() &&
                selectedRef->yamlIndex >= 0 &&
                selectedRef->yamlIndex < static_cast<int>(sequence.size())) {
                const YAML::Node actorNode = sequence[selectedRef->yamlIndex];
                isGeneratedUGCPlatform =
                    actorNode["ugcGeneratedPlatform"] &&
                    actorNode["ugcGeneratedPlatform"].as<bool>(false);
            }
        }
    }
    ImGui::TextUnformatted("高さを変える");
    const auto moveSelectedVertically =
        [this, selectedActor, selectedRef, isGeneratedUGCPlatform](float sign) {
        mEditCommandController.PushUndo();
        const float gridSize = mContext.game->GetUGCGridSize();
        const glm::vec3 movementDelta(0.0f, sign * gridSize, 0.0f);
        if (isGeneratedUGCPlatform && selectedRef) {
            if (mStageAddActorPanel.TryTranslateUGCPlatformCells(
                    *selectedRef, movementDelta)) {
                mSelectionController.Clear();
                mUGCStatus = sign > 0.0f
                    ? "足場を1だん上へ動かしました"
                    : "足場を1だん下へ動かしました";
            }
            return;
        }

        const glm::vec3 currentPosition = selectedActor->GetPos();
        const float snappedHeight =
            std::round(currentPosition.y / gridSize) * gridSize;
        const glm::vec3 targetPosition(
            currentPosition.x,
            snappedHeight + sign * gridSize,
            currentPosition.z);
        mSelectionController.MoveSelectedActorsByDelta(
            targetPosition - currentPosition);
        mStagePlacementPanel.SaveEditorAuthoredTransforms();
        mUGCStatus = sign > 0.0f
            ? "1だん上へ動かしました"
            : "1だん下へ動かしました";
    };
    if (ImGui::Button("1だん上へ", ImVec2(-1.0f, 40.0f))) {
        moveSelectedVertically(1.0f);
    }
    if (ImGui::Button("1だん下へ", ImVec2(-1.0f, 40.0f))) {
        moveSelectedVertically(-1.0f);
    }

    if (isGeneratedUGCPlatform) {
        ImGui::TextDisabled(
            "足場の向きと大きさはマスに合わせて自動で決まります。");
        return;
    }

    Platform* selectedPlatform = dynamic_cast<Platform*>(selectedActor);
    const bool isPressureSwitch = selectedPlatform && selectedRef &&
        selectedPlatform->GetPressureSwitchComponent() != nullptr;
    if (isPressureSwitch) {
        ImGui::Separator();
        ImGui::TextWrapped("このスイッチで表示する足場を選べます。");
        if (ImGui::Button("つなぐ", ImVec2(-1.0f, 40.0f))) {
            mUGCConnectionSwitchRef = *selectedRef;
            mSelectionController.Clear();
            mUGCStatus = "表示したい足場を1つクリックしてください";
        }
    }

    ImGui::Separator();
    ImGui::TextUnformatted("むきを変える");
    const auto rotateSelected = [this, selectedActor](float sign) {
        mEditCommandController.PushUndo();
        glm::vec3 rotation = selectedActor->GetEditorRotation();
        rotation.y += glm::radians(mUGCRotationStepDegrees * sign);
        selectedActor->SetEditorRotation(rotation);
        selectedActor->CaptureEditorAuthoredRotation();
        mStagePlacementPanel.SaveEditorAuthoredTransforms();
    };
    if (ImGui::Button("左に90度回す", ImVec2(-1.0f, 40.0f))) {
        rotateSelected(-1.0f);
    }
    if (ImGui::Button("右に90度回す", ImVec2(-1.0f, 40.0f))) {
        rotateSelected(1.0f);
    }

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
    pose.up = std::abs(normalizedDirection.y) > 0.9f
        ? glm::vec3(0.0f, 0.0f, -1.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
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
    // Keep the grid covering the viewport at every zoom level without
    // drawing thousands of sub-pixel lines. Placement still snaps to the
    // selected gridSize; only the visual grid becomes coarser when zoomed far
    // out, similar to DCC and game-engine editors.
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

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
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
