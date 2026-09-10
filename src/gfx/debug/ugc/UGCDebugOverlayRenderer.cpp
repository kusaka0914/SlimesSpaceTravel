#include <GL/glew.h>

#include "gfx/debug/ugc/UGCDebugOverlayRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/ugc/UGCEditorInteractionController.h"
#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCPreviewRenderer.h"
#include "gfx/debug/ugc/UGCSceneOverlayRenderer.h"
#include "gfx/debug/ugc/UGCWorkPanel.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>

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
        min, max, IM_COL32(255, 70, 45, 255), 7.0f, 0, 3.0f);
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
}

UGCDebugOverlayRenderer::UGCDebugOverlayRenderer(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    UGCEditorInteractionController& interactionController,
    UGCPreviewRenderer& previewRenderer,
    UGCSceneOverlayRenderer& sceneOverlayRenderer,
    UGCEditorToolState& toolState,
    UGCEditorMenuState& menuState,
    UGCWorkPanel& workPanel,
    EditorModelThumbnailRenderer* modelThumbnailRenderer,
    std::function<bool()> isAdjustingUGCUI)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mInteractionController(interactionController),
      mPreviewRenderer(previewRenderer),
      mSceneOverlayRenderer(sceneOverlayRenderer),
      mToolState(toolState),
      mMenuState(menuState),
      mWorkPanel(workPanel),
      mModelThumbnailRenderer(modelThumbnailRenderer),
      mIsAdjustingUGCUI(std::move(isAdjustingUGCUI))
{
}

void UGCDebugOverlayRenderer::Draw()
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
        mIsAdjustingUGCUI && mIsAdjustingUGCUI();
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
                    UGCControlLayout layout{
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
                    if (element.id == "play") {
                        constexpr float authoredGroupBottomRatio = 0.5755f;
                        constexpr float bottomMargin = 6.0f;
                        const float authoredBottomRatio =
                            element.yRatio + element.heightRatio;
                        const float adjustedBottomMargin = std::max(
                            0.0f,
                            bottomMargin + viewportSize.x *
                                (authoredGroupBottomRatio -
                                 authoredBottomRatio));
                        layout.position.y =
                            viewportMax.y - adjustedBottomMargin -
                            layout.size.y;
                    }
                    return layout;
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
            std::strcmp(id, "eraser") == 0 && mToolState.isEraserMode);
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
    if (mModelThumbnailRenderer) {
        mModelThumbnailRenderer->BeginFrame();
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
        const GLuint thumbnail = mModelThumbnailRenderer
            ? mModelThumbnailRenderer->ResolveThumbnail(
                  presetVisual.modelPath,
                  presetVisual.thumbnailScale,
                  presetVisual.initialTextureOverridePath)
            : 0;
        const bool isActive = mToolState.activePresetKind == preset.kind;
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
            mToolState.isEraserMode = false;
            if (mStageAddActorPanel.ActivateUGCPreset(preset.kind)) {
                mToolState.activePresetKind = preset.kind;
                mToolState.statusMessage = std::string(preset.label) + "を選びました";
            } else {
                mToolState.statusMessage = std::string(preset.label) +
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
            mMenuState.RequestWorkManagementOpen();
        }
        if (ImGui::MenuItem("完成チェック")) {
            mWorkPanel.StartVerification();
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
         mToolState.isEraserMode ? "消しゴム：ON" : "消しゴム",
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
            mInteractionController.ToggleEraser();
        } else if (std::strcmp(control.id, "undo") == 0) {
            mInteractionController.HandleUndo();
        } else if (std::strcmp(control.id, "redo") == 0) {
            mInteractionController.HandleRedo();
        } else if (std::strcmp(control.id, "layerUp") == 0) {
            mInteractionController.ChangeLayer(1);
        } else if (std::strcmp(control.id, "layerDown") == 0) {
            mInteractionController.ChangeLayer(-1);
        } else if (std::strcmp(control.id, "zoomIn") == 0) {
            mInteractionController.AdjustZoom(0.85f);
        } else if (std::strcmp(control.id, "zoomOut") == 0) {
            mInteractionController.AdjustZoom(1.18f);
        } else if (std::strcmp(control.id, "previewView") == 0) {
            mInteractionController.ToggleVerticalView();
        }
    }

    const UGCControlLayout playLayout = resolveUGCControlLayout(
        "play", ImVec2(viewportMin.x + 6.0f, viewportMax.y - 60.0f));
    ImGui::SetNextWindowPos(
        playLayout.position,
        ImGuiCond_Always);
    ImGui::PushStyleVar(
        ImGuiStyleVar_WindowPadding,
        ImVec2(0.0f, 0.0f));
    ImGui::Begin("###UGCDebugPlay", nullptr, overlayFlags);
    if (drawActionIcon(
            "play",
            "textures/ugc_ui/editor_action_play.png",
            "遊ぶ",
            playLayout.size)) {
        mContext.game->StartUGCPlaytest();
        ImGui::End();
        ImGui::PopStyleVar();
        return;
    }
    ImGui::End();
    ImGui::PopStyleVar();

    if (!mWorkPanel.IsManagementOpen()) {
        mSceneOverlayRenderer.DrawBackgroundGuides();
        mPreviewRenderer.DrawPreviewOverlay();
    }
    mWorkPanel.DrawManagement();
}
