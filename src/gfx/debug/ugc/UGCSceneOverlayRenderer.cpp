#include <GL/glew.h>

#include "gfx/debug/ugc/UGCSceneOverlayRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformLatchedGroupSwitchComponent.h"
#include "component/PlatformPressureSwitchComponent.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/stage/StagePlatformConnections.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "gfx/debug/ugc/UGCEditorInteractionController.h"
#include "gfx/debug/ugc/UGCSwitchConnectionController.h"
#include "system/CameraSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <yaml-cpp/yaml.h>

namespace {
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

UGCSceneOverlayRenderer::UGCSceneOverlayRenderer(
    DebugEditorContext& context,
    StageAddActorPanel& stageAddActorPanel,
    StageSelectionController& selectionController,
    UGCEditorInteractionController& interactionController,
    UGCSwitchConnectionController& connectionController,
    UGCEditorToolState& toolState,
    UGCSwitchConnectionState& connectionState)
    : mContext(context),
      mStageAddActorPanel(stageAddActorPanel),
      mSelectionController(selectionController),
      mInteractionController(interactionController),
      mConnectionController(connectionController),
      mToolState(toolState),
      mConnectionState(connectionState)
{
}

void UGCSceneOverlayRenderer::DrawBackgroundGuides()
{
    DrawGridOverlay();
    DrawStackBadges();
    DrawPlacementPreview();
}

void UGCSceneOverlayRenderer::DrawSelectionOverlays()
{
    DrawSwitchConnectionLines();
    DrawUnconnectedSwitchWarnings();
    DrawTransformControls();
}

void UGCSceneOverlayRenderer::DrawSwitchConnectionLines()
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

void UGCSceneOverlayRenderer::DrawUnconnectedSwitchWarnings()
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

void UGCSceneOverlayRenderer::DrawPlacementPreview()
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
    } else if (mConnectionState.GetSwitchRef()) {
        placementInstruction = mConnectionState.GetAction() ==
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

void UGCSceneOverlayRenderer::DrawLayerControls()
{
    ImGui::TextUnformatted("いま作っている高さ");
    if (ImGui::Button("▲ ひとつ上のだん", ImVec2(-1.0f, 44.0f))) {
        mInteractionController.ChangeEditLayer(1);
    }

    const std::string floorLabel =
        std::to_string(mToolState.editLayer + 1) + " だん目";
    const float labelWidth = ImGui::CalcTextSize(floorLabel.c_str()).x;
    ImGui::SetCursorPosX(std::max(
        ImGui::GetCursorPosX(),
        (ImGui::GetWindowWidth() - labelWidth) * 0.5f));
    ImGui::TextUnformatted(floorLabel.c_str());

    ImGui::BeginDisabled(mToolState.editLayer == 0);
    if (ImGui::Button("▼ ひとつ下のだん", ImVec2(-1.0f, 44.0f))) {
        mInteractionController.ChangeEditLayer(-1);
    }
    ImGui::EndDisabled();
    ImGui::TextWrapped(
        "上から見たまま、この高さだけを置いたり消したりできます。");
}

void UGCSceneOverlayRenderer::DrawTransformControls()
{
    Actor* selectedActor = mSelectionController.GetSingleSelectedActor();
    const std::optional<StageActorRef>& selectedRef =
        mSelectionController.GetPickedActorRef();
    if (mConnectionController.CompletePendingConnection(
            selectedActor,
            selectedRef)) {
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
            mConnectionController.BeginConnection(
                *selectedRef,
                UGCSwitchConnectionAction::Connect);
        }
        ImGui::SameLine(0.0f, buttonGap);
        if (ImGui::Button(
                "解除",
                ImVec2(buttonWidth, buttonHeight))) {
            mConnectionController.BeginConnection(
                *selectedRef,
                UGCSwitchConnectionAction::Disconnect);
        }
        ImGui::End();
    }
}

void UGCSceneOverlayRenderer::DrawGridOverlay()
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
        static_cast<float>(mToolState.editLayer) * gridSize,
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
    const glm::vec3 absoluteViewDirection =
        glm::abs(mInteractionController.GetViewDirection());
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

void UGCSceneOverlayRenderer::DrawStackBadges()
{
    if (!mContext.game || !mContext.gameViewport.IsValid() ||
        std::abs(mInteractionController.GetViewDirection().y) < 0.9f) {
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
            static_cast<float>(mToolState.editLayer) * gridSize,
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

