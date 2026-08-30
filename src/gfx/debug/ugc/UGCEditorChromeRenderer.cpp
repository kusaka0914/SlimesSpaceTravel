#include <GL/glew.h>

#include "gfx/debug/ugc/UGCEditorChromeRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/stage/UGCPresetVisuals.h"
#include "gfx/debug/ugc/UGCEditorInteractionController.h"
#include "gfx/debug/ugc/UGCEditorMenuState.h"
#include "gfx/debug/ugc/UGCEditorToolState.h"
#include "gfx/debug/ugc/UGCEditorTutorial.h"
#include "gfx/debug/ugc/UGCTutorialOverlayRenderer.h"
#include "gfx/debug/ugc/UGCWorkPanel.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

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
}

UGCEditorChromeRenderer::UGCEditorChromeRenderer(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    UGCEditorInteractionController& interactionController,
    UGCEditorTutorial& editorTutorial,
    UGCTutorialOverlayRenderer& tutorialOverlayRenderer,
    UGCEditorToolState& toolState,
    UGCEditorMenuState& menuState,
    UGCWorkPanel& workPanel,
    EditorModelThumbnailRenderer* modelThumbnailRenderer)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mInteractionController(interactionController),
      mEditorTutorial(editorTutorial),
      mTutorialOverlayRenderer(tutorialOverlayRenderer),
      mToolState(toolState),
      mMenuState(menuState),
      mWorkPanel(workPanel),
      mModelThumbnailRenderer(modelThumbnailRenderer)
{
}

void UGCEditorChromeRenderer::DrawSaveShortcut(
    const ImGuiViewport& viewport)
{
    if (mEditorTutorial.IsActive()) {
        return;
    }

    constexpr float referenceWidth = 2560.0f;
    constexpr float referenceHeight = 1440.0f;
    const float uiScale = std::max(
        1.0f,
        std::min(
            viewport.WorkSize.x / referenceWidth,
            viewport.WorkSize.y / referenceHeight));
    const float buttonWidth = 154.0f * uiScale;
    const float buttonHeight = 46.0f * uiScale;
    const float buttonGap = 12.0f * uiScale;
    const float tutorialButtonRightMargin = 230.0f * uiScale;
    const ImVec2 saveButtonPosition(
        viewport.WorkPos.x + viewport.WorkSize.x -
            tutorialButtonRightMargin - buttonGap - buttonWidth,
        viewport.WorkPos.y + 16.0f * uiScale);
    ImGui::SetNextWindowPos(saveButtonPosition, ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(buttonWidth, 76.0f * uiScale),
        ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
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
    constexpr ImGuiWindowFlags saveWindowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground;
    ImGui::Begin("###UGCSaveShortcut", nullptr, saveWindowFlags);
    ImGui::SetWindowFontScale(uiScale);
    if (ImGui::Button("保存", ImVec2(buttonWidth, buttonHeight))) {
        mWasLastSaveSuccessful = mWorkPanel.SaveCurrentWork();
        mSaveFeedbackMessage = mWasLastSaveSuccessful
            ? "保存しました"
            : "保存できませんでした";
        mSaveFeedbackRemainingSeconds = 2.5f;
    }

    if (mSaveFeedbackRemainingSeconds > 0.0f) {
        mSaveFeedbackRemainingSeconds = std::max(
            0.0f,
            mSaveFeedbackRemainingSeconds - ImGui::GetIO().DeltaTime);
        const ImVec2 messageSize =
            ImGui::CalcTextSize(mSaveFeedbackMessage.c_str());
        ImGui::SetCursorPosX(std::max(
            0.0f,
            (buttonWidth - messageSize.x) * 0.5f));
        const ImVec4 messageColor = mWasLastSaveSuccessful
            ? ImVec4(0.48f, 1.0f, 0.62f, 1.0f)
            : ImVec4(1.0f, 0.42f, 0.42f, 1.0f);
        ImGui::TextColored(
            messageColor,
            "%s",
            mSaveFeedbackMessage.c_str());
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

bool UGCEditorChromeRenderer::DrawControls()
{
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
    if (mModelThumbnailRenderer) {
        mModelThumbnailRenderer->BeginFrame();
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
        mToolState.activePresetKind == UGCPresetKind::NormalPlatform ||
        mToolState.activePresetKind == UGCPresetKind::MovingPlatform ||
        mToolState.activePresetKind == UGCPresetKind::FadingPlatform ||
        mToolState.activePresetKind == UGCPresetKind::AdhesivePlatform;
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
                mToolState.platformFootprintSideLength ==
                footprintSideLengths[index];
            if (ImGui::Selectable(
                    footprintLabels[index],
                    isSelected,
                    0,
                    ImVec2(52.0f, 0.0f))) {
                mToolState.platformFootprintSideLength =
                    footprintSideLengths[index];
                mStageAddActorPanel.SetUGCPlatformFootprintSideLength(
                    footprintSideLengths[index]);
            }
            mTutorialOverlayRenderer.DrawHighlightForLastItem(
                mEditorTutorial.ShouldHighlightFootprintOptions() &&
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
        const GLuint thumbnail = mModelThumbnailRenderer
            ? mModelThumbnailRenderer->ResolveThumbnail(
                  presetVisual.modelPath,
                  presetVisual.thumbnailScale,
                  presetVisual.initialTextureOverridePath)
            : 0;
        const bool isActivePreset = mToolState.activePresetKind == preset.kind;
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
        mTutorialOverlayRenderer.DrawHighlightForLastItem(
            mEditorTutorial.IsActive() &&
            mEditorTutorial.ShouldHighlightPreset(preset.kind));
        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", preset.label);
        }
        if (clicked) {
            mToolState.isEraserMode = false;
            const bool activated = mStageAddActorPanel.ActivateUGCPreset(preset.kind);
            if (activated) mToolState.activePresetKind = preset.kind;
            mToolState.statusMessage = activated ? std::string(preset.label) + "を選びました"
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
            std::strcmp(id, "eraser") == 0 && mToolState.isEraserMode);
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
        !mToolState.isEraserMode && !mStageAddActorPanel.IsPlacementActive();
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
        mInteractionController.ActivateSelectionMode();
    }
    mTutorialOverlayRenderer.DrawHighlightForLastItem(
        mEditorTutorial.ShouldHighlightSelection());
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
            mTutorialOverlayRenderer.DrawHighlightForLastItem(shouldHighlight);
            ImGui::End();
            ImGui::PopStyleVar();
            return clicked;
        };

    if (drawUGCActionControl(
            "eraser",
            "textures/ugc_ui/editor_action_eraser.png",
            mToolState.isEraserMode ? "消しゴム：ON" : "消しゴム",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 108.0f),
            mEditorTutorial.ShouldHighlightEraser())) {
        mInteractionController.ToggleEraser();
    }
    if (drawUGCActionControl(
            "undo",
            "textures/ugc_ui/editor_action_undo.png",
            "1つ戻す",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 172.0f),
            mEditorTutorial.ShouldHighlightUndo())) {
        mInteractionController.HandleUndo();
    }
    if (drawUGCActionControl(
            "redo",
            "textures/ugc_ui/editor_action_redo.png",
            "やり直す",
            ImVec2(gameViewportMax.x - 92.0f, gameViewportMin.y + 236.0f))) {
        mInteractionController.HandleRedo();
    }
    if (drawUGCActionControl(
            "layerUp",
            "textures/ugc_ui/editor_action_layer_up.png",
            "上のだん",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 16.0f),
            mEditorTutorial.ShouldHighlightLayerUp())) {
        mInteractionController.ChangeLayer(1);
    }
    if (drawUGCActionControl(
            "layerDown",
            "textures/ugc_ui/editor_action_layer_down.png",
            "下のだん",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 80.0f),
            mEditorTutorial.ShouldHighlightLayerDown())) {
        mInteractionController.ChangeLayer(-1);
    }
    if (drawUGCActionControl(
            "zoomIn",
            "textures/ugc_ui/editor_action_zoom_in.png",
            "近づく",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 152.0f),
            mEditorTutorial.ShouldHighlightZoom())) {
        mInteractionController.AdjustZoom(0.85f);
    }
    if (drawUGCActionControl(
            "zoomOut",
            "textures/ugc_ui/editor_action_zoom_out_control.png",
            "遠ざかる",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 216.0f),
            mEditorTutorial.ShouldHighlightZoom())) {
        mInteractionController.AdjustZoom(1.18f);
    }
    if (drawUGCActionControl(
            "previewView",
            "textures/ugc_ui/editor_action_preview_view_control.png",
            mContext.game->GetIsUGCPreviewViewedFromBelow()
                ? "上から見る"
                : "下から見る",
            ImVec2(gameViewportMin.x + 16.0f, gameViewportMin.y + topBarHeight + 280.0f))) {
        mInteractionController.ToggleVerticalView();
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
        mEditorTutorial.RecordPlaytestStarted();
        mContext.game->StartUGCPlaytest();
        ImGui::End();
        ImGui::PopStyleVar();
        return true;
    }
    mTutorialOverlayRenderer.DrawHighlightForLastItem(
        mEditorTutorial.ShouldHighlightPlaytest());
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
        mMenuState.ConsumeMenuOpenRequest() || wasMenuButtonPressed;
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
        if (mEditorTutorial.IsActive()) {
            ImGui::BeginDisabled();
        }
        ImGui::SetItemDefaultFocus();
        if (ImGui::Button(
                "保存・開く",
                ImVec2(menuButtonWidth, menuButtonHeight))) {
            mMenuState.RequestWorkManagementOpen();
            ImGui::CloseCurrentPopup();
        }
        if (ImGui::Button(
                "完成チェック",
                ImVec2(menuButtonWidth, menuButtonHeight))) {
            mWorkPanel.StartVerification();
            ImGui::CloseCurrentPopup();
        }
        if (mEditorTutorial.IsActive()) {
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
            if (mWorkPanel.HasUnsavedChanges()) {
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
    DrawSaveShortcut(*mainViewport);
    return false;
}
