#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
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

}

StageAddActorPanel::StageAddActorPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mCreateService(context)
{
    std::snprintf(mNPCName.data(), mNPCName.size(), "%s", "新しいNPC");
    mNPCTalkTexts.emplace_back();
    std::snprintf(
        mNPCTalkTexts.front().data(),
        mNPCTalkTexts.front().size(),
        "%s",
        "こんにちは");
    mTutorialTriggerTalkTexts.emplace_back();
    std::snprintf(
        mTutorialTriggerTalkTexts.front().data(),
        mTutorialTriggerTalkTexts.front().size(),
        "%s",
        "ここにチュートリアルの内容を入力");
}

void StageAddActorPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

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

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
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

            if (ImGui::Button("乗ると動く足場を追加")) {
                const bool created =
                    mCreateService.AddRideMovingPlatform(
                        mSelectedPlatformPlanetIndex,
                        platformModels[mSelectedPlatformModelIndex],
                        mPlatformScale);
                mRideMovingPlatformStatus =
                    created
                        ? "乗降式の動く足場を追加しました"
                        : "動く足場の追加に失敗しました";
            }

            if (!canAddPlatform) {
                ImGui::EndDisabled();
            }

            if (!mRideMovingPlatformStatus.empty()) {
                ImGui::TextUnformatted(mRideMovingPlatformStatus.c_str());
            }
            ImGui::TextDisabled(
                "追加後は「配置」から出発地点と到着地点を調整できます。");

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

            ImGui::InputTextWithHint(
                "##npcModelSearch",
                "NPCモデル名を検索",
                mNPCModelSearch.data(),
                mNPCModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText = ToLower(mNPCModelSearch.data());

            ImGui::BeginChild("NPCModelAssetPicker", ImVec2(0.0f, 180.0f), true);
            for (const std::string& modelPath : modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) == std::string::npos) {
                    continue;
                }

                const bool selected = modelPath == mSelectedNPCModel;
                if (ImGui::Selectable(modelPath.c_str(), selected)) {
                    mSelectedNPCModel = modelPath;
                }
            }
            ImGui::EndChild();

            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedNPCModel.empty() ? "未選択" : mSelectedNPCModel.c_str());
            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");

            ImGui::InputText("NPC名", mNPCName.data(), mNPCName.size());
            ImGui::DragFloat(
                "初期スケール",
                &mNPCScale,
                0.01f,
                0.01f,
                30.0f,
                "%.2f");
            ImGui::DragFloat(
                "会話できる距離",
                &mNPCTalkRadius,
                0.05f,
                0.1f,
                20.0f,
                "%.2f");

            ImGui::SeparatorText("会話内容");
            for (std::size_t talkIndex = 0;
                 talkIndex < mNPCTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "会話 " + std::to_string(talkIndex + 1) +
                    "##newNPCTalk" + std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mNPCTalkTexts[talkIndex].data(),
                    mNPCTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mNPCTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("この会話を削除##newNPCTalkDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mNPCTalkTexts.erase(
                        mNPCTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(talkIndex));
                    break;
                }
            }

            if (ImGui::Button("会話を追加##newNPC")) {
                mNPCTalkTexts.emplace_back();
            }

            const bool canAddNPC =
                mSelectedNPCPlanetIndex >= 0 &&
                !mSelectedNPCModel.empty();

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星とモデルを選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(mNPCTalkTexts.size());
                for (const auto& talkText : mNPCTalkTexts) {
                    talkTexts.emplace_back(talkText.data());
                }

                const bool created = mCreateService.AddNPC(
                    mSelectedNPCModel,
                    mSelectedNPCPlanetIndex,
                    mNPCName.data(),
                    talkTexts,
                    mNPCTalkRadius,
                    mNPCScale);
                mNPCStatus =
                    created ? "NPCを追加しました" : "NPCの追加に失敗しました";
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            if (!mNPCStatus.empty()) {
                ImGui::TextUnformatted(mNPCStatus.c_str());
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("チュートリアルトリガー追加")) {
        const auto& planets =
            mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::TextUnformatted(
                "追加先の惑星がありません");
        } else {
            DrawPlanetCombo(
                "追加先の惑星##tutorialTrigger",
                mSelectedTutorialTriggerPlanetIndex);

            ImGui::InputTextWithHint(
                "##tutorialTriggerModelSearch",
                "箱型モデルを検索",
                mTutorialTriggerModelSearch.data(),
                mTutorialTriggerModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText =
                ToLower(
                    mTutorialTriggerModelSearch.data());
            ImGui::BeginChild(
                "TutorialTriggerModelAssetPicker",
                ImVec2(0.0f, 180.0f),
                true);
            for (const std::string& modelPath :
                 modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) ==
                        std::string::npos) {
                    continue;
                }

                const bool selected =
                    modelPath ==
                    mSelectedTutorialTriggerModel;
                if (ImGui::Selectable(
                        modelPath.c_str(),
                        selected)) {
                    mSelectedTutorialTriggerModel =
                        modelPath;
                }
            }
            ImGui::EndChild();
            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedTutorialTriggerModel.empty()
                    ? "未選択"
                    : mSelectedTutorialTriggerModel.c_str());

            ImGui::DragFloat3(
                "初期スケール##tutorialTrigger",
                &mTutorialTriggerScale.x,
                0.05f,
                0.01f,
                100.0f,
                "%.2f");

            ImGui::SeparatorText("チュートリアル内容");
            for (std::size_t talkIndex = 0;
                 talkIndex <
                 mTutorialTriggerTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "ページ " +
                    std::to_string(talkIndex + 1) +
                    "##newTutorialTriggerTalk" +
                    std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mTutorialTriggerTalkTexts[talkIndex].data(),
                    mTutorialTriggerTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mTutorialTriggerTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("このページを削除##newTutorialTriggerDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mTutorialTriggerTalkTexts.erase(
                        mTutorialTriggerTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(
                            talkIndex));
                    break;
                }
            }

            if (ImGui::Button(
                    "ページを追加##newTutorialTrigger")) {
                mTutorialTriggerTalkTexts.emplace_back();
            }

            const bool canAdd =
                mSelectedTutorialTriggerPlanetIndex >= 0 &&
                !mSelectedTutorialTriggerModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(
                    "チュートリアルトリガーを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(
                    mTutorialTriggerTalkTexts.size());
                for (const auto& talkText :
                     mTutorialTriggerTalkTexts) {
                    talkTexts.emplace_back(
                        talkText.data());
                }

                const bool created =
                    mCreateService.AddTutorialTrigger(
                        mSelectedTutorialTriggerPlanetIndex,
                        mSelectedTutorialTriggerModel,
                        talkTexts,
                        mTutorialTriggerScale);
                mTutorialTriggerStatus =
                    created
                        ? "チュートリアルトリガーを追加しました"
                        : "チュートリアルトリガーの追加に失敗しました";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mTutorialTriggerStatus.empty()) {
                ImGui::TextUnformatted(
                    mTutorialTriggerStatus.c_str());
            }
            ImGui::TextDisabled(
                "箱型モデルを使うと、モデルの位置・回転・スケールと反応範囲が一致します。");
            ImGui::TextDisabled(
                "ゲーム中は見えず、衝突しません。内部に入ると一度だけ開始します。");
        }

        ImGui::TreePop();
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
