#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>
#include <vector>

namespace {
std::string ToLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

bool IsSupportedModelExtension(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    return extension == ".obj" || extension == ".fbx" || extension == ".gltf" ||
           extension == ".glb" || extension == ".dae";
}

std::vector<std::string> CollectModelAssets()
{
    std::vector<std::string> modelPaths;
    const std::filesystem::path modelDirectory("../assets/models");
    std::error_code error;

    for (std::filesystem::recursive_directory_iterator it(modelDirectory, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error) || !IsSupportedModelExtension(it->path())) {
            continue;
        }

        const std::filesystem::path relativePath =
            std::filesystem::relative(it->path(), modelDirectory, error);
        if (error) {
            error.clear();
            continue;
        }

        modelPaths.emplace_back(relativePath.generic_string());
    }

    std::sort(modelPaths.begin(), modelPaths.end());
    return modelPaths;
}
}

StageAddActorPanel::StageAddActorPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mCreateService(context)
{
}

void StageAddActorPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (ImGui::TreeNode("汎用モデル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
        if (planets.empty()) {
            ImGui::TextUnformatted("惑星が存在しないため、モデルを追加できません");
        } else {
            DrawPlanetCombo("追加先の惑星##stageObject", mSelectedStageObjectPlanetIndex);
            ImGui::InputTextWithHint(
                "##stageObjectSearch",
                "モデル名を検索",
                mStageObjectSearch.data(),
                mStageObjectSearch.size());

            const std::vector<std::string> modelAssets = CollectModelAssets();
            const std::string searchText = ToLower(mStageObjectSearch.data());

            ImGui::BeginChild("StageObjectAssetPicker", ImVec2(0.0f, 180.0f), true);
            for (const std::string& modelPath : modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) == std::string::npos) {
                    continue;
                }

                const bool selected = modelPath == mSelectedStageObjectModel;
                if (ImGui::Selectable(modelPath.c_str(), selected)) {
                    mSelectedStageObjectModel = modelPath;
                }
            }
            ImGui::EndChild();

            ImGui::Text(
                "選択中: %s",
                mSelectedStageObjectModel.empty()
                    ? "未選択"
                    : mSelectedStageObjectModel.c_str());
            ImGui::Checkbox("モデル形状の当たり判定を作る", &mStageObjectCollisionEnabled);

            const bool canAdd =
                mSelectedStageObjectPlanetIndex >= 0 &&
                !mSelectedStageObjectModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("選択したモデルをステージに追加")) {
                const bool created = mCreateService.AddStageObject(
                    mSelectedStageObjectPlanetIndex,
                    mSelectedStageObjectModel,
                    mStageObjectCollisionEnabled);
                mStageObjectStatus =
                    created ? "モデルを追加しました" : "モデルの追加に失敗しました";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mStageObjectStatus.empty()) {
                ImGui::SameLine();
                ImGui::TextUnformatted(mStageObjectStatus.c_str());
            }

            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");
        }
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("惑星追加")) {
        const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
        const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

        ImGui::Combo("惑星モデル", &mSelectedPlanetModelIndex, planetModelLabels, IM_ARRAYSIZE(planetModelLabels));

        if (ImGui::Button("惑星を追加")) {
            mCreateService.AddPlanet(planetModels[mSelectedPlanetModelIndex]);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("敵追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、敵を追加できません");
            ImGui::TreePop();
            return;
        }

        DrawPlanetCombo("敵の追加先惑星", mSelectedEnemyPlanetIndex);

        const char* enemyTypeLabels[] = {"通常敵", "ボス敵", "動かない敵", "動かない大きい敵"};
        const char* enemyTypes[] = {"normal", "boss", "normalFixed", "bigFixed"};

        ImGui::Combo("敵タイプ", &mSelectedEnemyTypeIndex, enemyTypeLabels, IM_ARRAYSIZE(enemyTypeLabels));

        const bool canAddEnemy = mSelectedEnemyPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            mCreateService.AddEnemy(enemyTypes[mSelectedEnemyTypeIndex], mSelectedEnemyPlanetIndex);
        }

        if (!canAddEnemy) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("足場追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、足場を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("追加先の惑星##platform", mSelectedPlatformPlanetIndex);

            const char* platformModelLabels[] = {"通常足場", "カーブ足場", "細い足場"};
            const char* platformModels[] = {"platform.obj", "curvePlatform.obj", "platform_thin.obj"};

            ImGui::Combo("モデル##platform", &mSelectedPlatformModelIndex, platformModelLabels,
                         IM_ARRAYSIZE(platformModelLabels));

            ImGui::SliderFloat("スケールX##platform", &mPlatformScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &mPlatformScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &mPlatformScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = mSelectedPlatformPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                mCreateService.AddPlatform(mSelectedPlatformPlanetIndex, platformModels[mSelectedPlatformModelIndex],
                                           mPlatformScale);
            }

            if (!canAddPlatform) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("クリスタル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、クリスタルを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("クリスタルの追加先惑星", mSelectedCrystalPlanetIndex);

            const char* crystalTypeLabels[] = {"小さいクリスタル", "大きいクリスタル"};
            const char* crystalTypes[] = {"little", "big"};

            ImGui::Combo("クリスタルタイプ", &mSelectedCrystalTypeIndex, crystalTypeLabels,
                         IM_ARRAYSIZE(crystalTypeLabels));

            const bool canAddCrystal = mSelectedCrystalPlanetIndex >= 0;

            if (!canAddCrystal) {
                ImGui::Text("クリスタルを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("クリスタルを追加")) {
                mCreateService.AddCrystal(crystalTypes[mSelectedCrystalTypeIndex], mSelectedCrystalPlanetIndex);
            }

            if (!canAddCrystal) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("NPC追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、NPCを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("NPCの追加先惑星", mSelectedNPCPlanetIndex);

            const char* npcTypeLabels[] = {"宇宙スライム", "母スライム", "プレイヤー型", "悪い母スライム",
                                           "博士スライム"};

            const char* npcTypes[] = {"spaceSlime", "motherSlime", "player", "badMotherSlime", "doctorSlime"};

            ImGui::Combo("NPCタイプ", &mSelectedNPCTypeIndex, npcTypeLabels, IM_ARRAYSIZE(npcTypeLabels));

            const bool canAddNPC = mSelectedNPCPlanetIndex >= 0;

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                mCreateService.AddNPC(npcTypes[mSelectedNPCTypeIndex], mSelectedNPCPlanetIndex);
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボートパーツ追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートパーツを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートパーツの追加先惑星", mSelectedBoatPartsPlanetIndex);

            const char* boatPartsTypeLabels[] = {"パーツ1", "パーツ2", "パーツ3", "パーツ4", "パーツ5"};
            const char* boatPartsTypes[] = {"parts1", "parts2", "parts3", "parts4", "parts5"};

            ImGui::Combo("ボートパーツタイプ", &mSelectedBoatPartsTypeIndex, boatPartsTypeLabels,
                         IM_ARRAYSIZE(boatPartsTypeLabels));

            const bool canAddBoatParts = mSelectedBoatPartsPlanetIndex >= 0;

            if (!canAddBoatParts) {
                ImGui::Text("ボートパーツを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートパーツを追加")) {
                mCreateService.AddBoatParts(boatPartsTypes[mSelectedBoatPartsTypeIndex], mSelectedBoatPartsPlanetIndex);
            }

            if (!canAddBoatParts) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボート追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートの開始惑星", mSelectedBoatStartPlanetIndex);
            DrawPlanetCombo("ボートの移動先惑星", mSelectedBoatDestPlanetIndex);

            ImGui::InputInt("移動先ステージ", &mSelectedBoatDestStage);

            const bool canAddBoat = mSelectedBoatStartPlanetIndex >= 0 && mSelectedBoatDestPlanetIndex >= 0;

            if (!canAddBoat) {
                ImGui::Text("ボートを追加するには、開始惑星と移動先惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートを追加")) {
                mCreateService.AddBoat(mSelectedBoatStartPlanetIndex, mSelectedBoatDestPlanetIndex,
                                       mSelectedBoatDestStage);
            }

            if (!canAddBoat) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("星追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、星を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("星の追加先惑星", mSelectedStarPlanetIndex);

            const bool canAddStar = mSelectedStarPlanetIndex >= 0;

            if (!canAddStar) {
                ImGui::Text("星を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("星を追加")) {
                mCreateService.AddStar(mSelectedStarPlanetIndex);
            }

            if (!canAddStar) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }
}

void StageAddActorPanel::DrawPlanetCombo(const char* label, int& selectedPlanetIndex)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        selectedPlanetIndex = -1;
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (selectedPlanetIndex >= static_cast<int>(planets.size())) {
        selectedPlanetIndex = -1;
    }

    std::string previewText = "未選択";
    if (selectedPlanetIndex >= 0) {
        previewText = "惑星 " + std::to_string(selectedPlanetIndex);
    }

    if (ImGui::BeginCombo(label, previewText.c_str())) {
        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            Planet* planet = planets[i];
            if (!planet) {
                continue;
            }

            std::string itemLabel = "惑星 " + std::to_string(i);
            bool isSelected = selectedPlanetIndex == i;

            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                selectedPlanetIndex = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}
