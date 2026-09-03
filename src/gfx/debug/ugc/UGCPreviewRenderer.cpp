#include <GL/glew.h>

#include "gfx/debug/ugc/UGCPreviewRenderer.h"

#include "Game.h"
#include "gfx/UIRenderer.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/CameraSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <set>
#include <yaml-cpp/yaml.h>

UGCPreviewRenderer::UGCPreviewRenderer(
    DebugEditorContext& context,
    UGCEditorToolState& toolState,
    std::function<bool()> isAdjustingUGCUI)
    : mContext(context),
      mToolState(toolState),
      mIsAdjustingUGCUI(std::move(isAdjustingUGCUI))
{
}

void UGCPreviewRenderer::DrawGameViewport(
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

void UGCPreviewRenderer::DrawPreviewOverlay()
{
    if (!mContext.game || !mContext.gameViewport.IsValid()) {
        return;
    }
    const unsigned int previewTexture =
        mContext.game->GetUGCPreviewTexture();
    if (previewTexture == 0) {
        return;
    }

    constexpr float previewBottomMargin = 14.0f;
    constexpr float previewHeaderHeight = 24.0f;
    const float maximumWidthFromViewportHeight = std::max(
        1.0f,
        (mContext.gameViewport.height - previewBottomMargin -
         previewHeaderHeight) * 16.0f / 9.0f);
    const float maximumPreviewWidth = std::max(
        1.0f,
        std::min(
            mContext.gameViewport.width * 0.5f,
            maximumWidthFromViewportHeight));
    const float minimumPreviewWidth =
        std::min(180.0f, maximumPreviewWidth);
    if (!mPanelState.HasInitializedWidth()) {


        mPanelState.InitializeWidth(
            maximumPreviewWidth * (2.0f / 3.0f),
            minimumPreviewWidth,
            maximumPreviewWidth);
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
        mPanelState.GetWidth(),
        minimumPreviewWidth,
        maximumPreviewWidth);
    float previewHeight = previewWidth * 9.0f / 16.0f;
    ImVec2 previewMin(
        mContext.gameViewport.x +
            mContext.gameViewport.width - previewWidth - 14.0f,
        mContext.gameViewport.y +
            mContext.gameViewport.height - previewHeight -
            previewBottomMargin);
    if (previewElement) {
        const float configuredPreviewWidth = std::max(
            1.0f,
            mContext.gameViewport.width *
                previewElement->widthRatio);
        previewWidth = std::min(
            configuredPreviewWidth,
            maximumPreviewWidth);
        const float configuredAspectRatio =
            previewElement->widthRatio /
            std::max(previewElement->heightRatio, 0.0001f);
        previewHeight = previewWidth /
            std::max(configuredAspectRatio, 0.0001f);
        const float configuredX =
            mContext.gameViewport.x +
            mContext.gameViewport.width * previewElement->xRatio;
        constexpr float authoredPreviewBottomRatio = 0.57f;
        const float authoredBottomRatio =
            previewElement->yRatio + previewElement->heightRatio;
        const float adjustedBottomMargin = std::max(
            0.0f,
            previewBottomMargin + mContext.gameViewport.width *
                (authoredPreviewBottomRatio - authoredBottomRatio));
        const float minimumX = mContext.gameViewport.x + 14.0f;
        const float maximumX =
            mContext.gameViewport.x + mContext.gameViewport.width -
            previewWidth - 14.0f;
        previewMin = ImVec2(
            glm::clamp(configuredX, minimumX, std::max(minimumX, maximumX)),
            mContext.gameViewport.y + mContext.gameViewport.height -
                previewHeight - adjustedBottomMargin);
    }
    mPanelState.SetWidth(
        previewWidth,
        minimumPreviewWidth,
        maximumPreviewWidth);
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
        mIsAdjustingUGCUI && mIsAdjustingUGCUI();
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
    DrawPreviewLayerGuides(previewMin, previewMax, drawList);
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
        mPanelState.BeginResize();
    }
    if (ImGui::IsItemActive() && !isAdjustingUGCUI) {
        const float horizontalDrag =
            ImGui::GetMouseDragDelta(ImGuiMouseButton_Left).x;
        mPanelState.Resize(
            horizontalDrag,
            minimumPreviewWidth,
            maximumPreviewWidth);
        if (previewElement) {
            const float previousWidthRatio =
                previewElement->widthRatio;
            const float previousHeightRatio =
                previewElement->heightRatio;
            const float resizedWidthRatio =
                mPanelState.GetWidth() /
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

void UGCPreviewRenderer::DrawPreviewLayerGuides(
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
        if (layer == mToolState.editLayer) {
            continue;
        }
        drawLayerLabel(layer, IM_COL32(160, 225, 255, 240));
    }

    const float currentLayerY =
        static_cast<float>(mToolState.editLayer) * gridSize;
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
    drawLayerLabel(mToolState.editLayer, IM_COL32(255, 220, 105, 255));
    drawList->PopClipRect();
}
