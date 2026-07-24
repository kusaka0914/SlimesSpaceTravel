#include "gfx/debug/panels/StageEditorPanel.h"

#include "Game.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
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
    DrawStageSwitcher();
    ImGui::Separator();

    const char* menus[] = {"追加", "配置", "削除"};

    ImGui::BeginChild("StageEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mSelectedMenu == i)) {
            mSelectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する", ImVec2(-1, 0))) {
        mPlanetPanel.Save();
        mPlacementPanel.Save();
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("StageEditorRight", ImVec2(0, 0), true);

    switch (mSelectedMenu) {
    case 0:
        mAddActorPanel.Draw();
        break;
    case 1:
        mPlanetPanel.Draw();
        mPlacementPanel.Draw();
        break;
    case 2:
        mDeleteActorPanel.Draw();
        break;
    default:
        break;
    }

    ImGui::EndChild();
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
    mSelectedMenu = 1;
    mPlacementPanel.RequestOpenPickedActorPlacement();
}

bool StageEditorPanel::ConsumeRequestOpenMainTab()
{
    const bool result = mRequestOpenMainTab;
    mRequestOpenMainTab = false;
    return result;
}
