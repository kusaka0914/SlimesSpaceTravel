#include "gfx/debug/panels/StageEditorPanel.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/DebugEditorLayout.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include "imgui.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace {
struct StageYamlOption {
    std::string label;
    std::string path;
    int stageNumber = 0;
};

int InferStageNumber(const std::string& stem, int currentStageNumber)
{
    if (stem == "house") {
        return 0;
    }
    if (stem == "test") {
        return 1;
    }

    if (stem.rfind("stage", 0) == 0 && stem.size() > 5) {
        try {
            return std::stoi(stem.substr(5));
        } catch (const std::exception&) {
        }
    }

    return currentStageNumber;
}

std::vector<StageYamlOption> CollectStageYamlOptions(int currentStageNumber, int stageCount)
{
    std::vector<StageYamlOption> options;
    const std::filesystem::path stageDirectory("../assets/data/stage");
    std::error_code error;

    for (std::filesystem::directory_iterator it(stageDirectory, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error) || it->path().extension() != ".yaml") {
            continue;
        }

        const std::string stem = it->path().stem().string();
        const int stageNumber = InferStageNumber(stem, currentStageNumber);
        if (stageNumber < 0 || stageNumber >= stageCount) {
            continue;
        }

        StageYamlOption option;
        option.label = stem + "（ステージ " + std::to_string(stageNumber) + "）";
        option.path = it->path().generic_string();
        option.stageNumber = stageNumber;
        options.emplace_back(std::move(option));
    }

    std::sort(
        options.begin(),
        options.end(),
        [](const StageYamlOption& left, const StageYamlOption& right) {
            if (left.stageNumber != right.stageNumber) {
                return left.stageNumber < right.stageNumber;
            }
            return left.label < right.label;
        });
    return options;
}
}

StageEditorPanel::StageEditorPanel(DebugEditorContext& context, StageAddActorPanel& addActorPanel,
                                   StagePlanetPanel& planetPanel, StagePlacementPanel& placementPanel,
                                   StageDeleteActorPanel& deleteActorPanel,
                                   StageSelectionController& selectionController)
    : DebugPanel(context),
      mAddActorPanel(addActorPanel),
      mPlanetPanel(planetPanel),
      mPlacementPanel(placementPanel),
      mDeleteActorPanel(deleteActorPanel),
      mSelectionController(selectionController)
{
}

void StageEditorPanel::Draw()
{
    DrawWorkspaceWindows();
}

void StageEditorPanel::DrawTopBar()
{
    DrawToolbar();
}

void StageEditorPanel::DrawToolbar()
{
    constexpr const char* menuLabels[] = {
        "デバッグ状態",
        "追加",
        "選択中",
        "削除",
    };
    constexpr int menuValues[] = {0, 1, 3, 6};

    ImGui::Separator();
    for (int menuIndex = 0; menuIndex < IM_ARRAYSIZE(menuLabels); ++menuIndex) {
        if (menuIndex > 0) {
            ImGui::SameLine();
        }
        if (ImGui::Selectable(
                menuLabels[menuIndex],
                mSelectedMenu == menuValues[menuIndex],
                0,
                ImVec2(92.0f, 0.0f))) {
            mSelectedMenu = menuValues[menuIndex];
        }
    }

    if (mAddActorPanel.IsPlacementActive()) {
        ImGui::SameLine();
        if (ImGui::Button("追加解除##topBar")) {
            mAddActorPanel.CancelPlacement();
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("ステージを保存")) {
        mPlanetPanel.Save();
        mPlacementPanel.Save();
    }
}

void StageEditorPanel::DrawWorkspaceWindows()
{
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImVec2 workPosition = viewport->WorkPos;
    const ImVec2 workSize = viewport->WorkSize;
    const float workspaceTop =
        workPosition.y + DebugEditorLayout::StageTopBarHeight;

    const float hierarchyWidth =
        DebugEditorLayout::CalculateHierarchyWidth(workSize.x);
    const float inspectorWidth =
        mContext.layout.rightPanelWidth > 0.0f
            ? mContext.layout.rightPanelWidth
            : DebugEditorLayout::CalculateToolPanelWidth(workSize.x);
    const float workspaceHeight =
        std::max(120.0f, workPosition.y + workSize.y - workspaceTop);

    constexpr ImGuiWindowFlags panelFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowPos(
        ImVec2(workPosition.x, workspaceTop),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(hierarchyWidth, workspaceHeight),
        ImGuiCond_Always);
    ImGui::Begin("オブジェクト一覧###StageHierarchy", nullptr, panelFlags);
    mPlacementPanel.DrawObjectList();
    ImGui::End();

    ImGui::SetNextWindowPos(
        ImVec2(workPosition.x + workSize.x - inspectorWidth, workspaceTop),
        ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(inspectorWidth, workspaceHeight),
        ImGuiCond_Always);
    ImGui::Begin("インスペクター###StageInspector", nullptr, panelFlags);
    DrawInspector();
    ImGui::End();
}

void StageEditorPanel::DrawInspector()
{
    switch (mSelectedMenu) {
    case 0:
        DrawStageSwitcher();
        DrawStageClearProgressEditor();
        break;
    case 1:
        mAddActorPanel.Draw();
        break;
    case 3:
        if (Planet* selectedPlanet = dynamic_cast<Planet*>(
                mSelectionController.GetSingleSelectedActor())) {
            mPlanetPanel.DrawSelectedPlanet(selectedPlanet);
            ImGui::SeparatorText("プレイヤースポーン");
            mPlacementPanel.DrawPlayerSpawn();
        } else {
            DrawDuplicatePlacementControls();
            mPlacementPanel.Draw();
        }
        break;
    case 6:
        mDeleteActorPanel.Draw();
        break;
    default:
        mPlacementPanel.Draw();
        break;
    }
}

void StageEditorPanel::DrawDuplicatePlacementControls()
{
    ImGui::SeparatorText("クリック複製");

    if (mAddActorPanel.IsPlacementActive()) {
        ImGui::TextDisabled(
            "連続配置中です。ゲーム画面をクリックして配置します。");
        if (ImGui::Button("配置解除##duplicatePlacement")) {
            mAddActorPanel.CancelPlacement();
            mDuplicatePlacementStatus = "連続配置を終了しました";
        }
        return;
    }

    const bool hasSingleSelection =
        mSelectionController.GetSelectedActorCount() == 1;
    if (!hasSingleSelection) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button(
            "選択中の設定で連続配置",
            ImVec2(-1.0f, 0.0f))) {
        Actor* selectedActor =
            mSelectionController.GetSingleSelectedActor();
        const std::optional<StageActorRef> selectedActorRef =
            StageActorQuery::FindTargetForActor(
                mContext.game ? mContext.game->GetCurrentStage() : nullptr,
                selectedActor);

        if (!selectedActorRef) {
            mDuplicatePlacementStatus =
                "選択中オブジェクトの保存データを取得できませんでした";
        } else {
            // Capture the current inspector values before the YAML node is
            // retained as the repeated-placement template.
            mPlacementPanel.Save();
            const bool started =
                mAddActorPanel.BeginDuplicatePlacement(
                    *selectedActorRef);
            mDuplicatePlacementStatus =
                started
                    ? "ゲーム画面をクリックすると同じ設定で配置できます"
                    : "選択中オブジェクトの複製準備に失敗しました";
        }
    }

    if (!hasSingleSelection) {
        ImGui::EndDisabled();
        ImGui::TextDisabled(
            "複製元にするオブジェクトを1つだけ選択してください。");
    } else {
        ImGui::TextDisabled(
            "モデルやコンポーネントを維持し、位置と所属惑星をクリック先へ変更します。");
    }

    if (!mDuplicatePlacementStatus.empty()) {
        ImGui::TextWrapped(
            "%s", mDuplicatePlacementStatus.c_str());
    }
}

void StageEditorPanel::DrawStageClearProgressEditor()
{
    if (!mContext.game) {
        return;
    }

    ImGui::SeparatorText("ステージクリア状況");
    ImGui::TextDisabled(
        "チェックを変更すると即座に反映され、次回起動時にも保持されます。");
    ImGui::TextDisabled(
        "NPC会話・頭上の一言・クリア条件付きオブジェクトの判定に使用されます。");

    const int currentStageNum = mContext.game->GetCurrentStageNum();
    const int stageCount =
        static_cast<int>(mContext.game->GetStages().size());
    for (int stageNum = 0; stageNum < stageCount; ++stageNum) {
        bool isCleared = mContext.game->IsStageCleared(stageNum);
        const std::string checkboxLabel =
            "ステージ " + std::to_string(stageNum) +
            " をクリア済みにする##stageClearProgress" +
            std::to_string(stageNum);
        if (ImGui::Checkbox(checkboxLabel.c_str(), &isCleared)) {
            mContext.game->SetStageCleared(stageNum, isCleared);
        }

        if (stageNum == currentStageNum) {
            ImGui::SameLine();
            ImGui::TextDisabled("（現在編集中）");
        }
    }
}

void StageEditorPanel::DrawStageSwitcher()
{
    if (!mContext.game) {
        return;
    }

    const int currentStageNumber = mContext.game->GetCurrentStageNum();
    const std::string& currentYamlPath = mContext.game->GetCurrentStageYamlPath();
    const std::vector<StageYamlOption> options = CollectStageYamlOptions(
        currentStageNumber,
        static_cast<int>(mContext.game->GetStages().size()));

    if (mSelectedStageYamlPath.empty()) {
        mSelectedStageYamlPath = currentYamlPath;
    }

    const StageYamlOption* selectedOption = nullptr;
    for (const StageYamlOption& option : options) {
        if (option.path == mSelectedStageYamlPath) {
            selectedOption = &option;
            break;
        }
    }

    ImGui::SeparatorText("ステージ切り替え");
    ImGui::Text(
        "現在: ステージ %d / %s",
        currentStageNumber,
        currentYamlPath.c_str());

    const char* previewLabel = selectedOption ? selectedOption->label.c_str() : "ステージを選択";
    if (ImGui::BeginCombo("読込データ", previewLabel)) {
        for (const StageYamlOption& option : options) {
            const bool selected = option.path == mSelectedStageYamlPath;
            if (ImGui::Selectable(option.label.c_str(), selected)) {
                mSelectedStageYamlPath = option.path;
            }
        }
        ImGui::EndCombo();
    }

    selectedOption = nullptr;
    for (const StageYamlOption& option : options) {
        if (option.path == mSelectedStageYamlPath) {
            selectedOption = &option;
            break;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("選択したステージへ移動") && selectedOption) {
        if (mContext.game->DebugChangeStage(
                selectedOption->stageNumber,
                selectedOption->path)) {
            mAddActorPanel.CancelPlacement();
            mSelectionController.Clear();
            mSelectedStageYamlPath = mContext.game->GetCurrentStageYamlPath();
            mStageSwitchStatus = "ステージを切り替えました";
        } else {
            mStageSwitchStatus = "ステージ切り替えに失敗しました";
        }
    }

    if (!mStageSwitchStatus.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mStageSwitchStatus.c_str());
    }
    ImGui::TextDisabled("切り替えると、保存していない現在のステージ編集内容は失われます。");
}

void StageEditorPanel::RequestOpenPlacementTab()
{
    mRequestOpenMainTab = true;
    mSelectedMenu = 3;
    mPlacementPanel.RequestOpenPickedActorPlacement();
}

bool StageEditorPanel::ConsumeRequestOpenMainTab()
{
    const bool result = mRequestOpenMainTab;
    mRequestOpenMainTab = false;
    return result;
}
