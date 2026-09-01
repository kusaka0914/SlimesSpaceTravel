#include "gfx/debug/stage/StageActorCreationForms.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageCreationFormWidgets.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

namespace {

std::string ToLower(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return text;
}


}

StageEnemyCreationForm::StageEnemyCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StageNPCCreationForm::StageNPCCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
    std::snprintf(mName.data(), mName.size(), "%s", "新しいNPC");
    mTalkTexts.emplace_back();
    std::snprintf(
        mTalkTexts.front().data(),
        mTalkTexts.front().size(),
        "%s",
        "こんにちは");
}

StageTutorialTriggerCreationForm::StageTutorialTriggerCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
    mTalkTexts.emplace_back();
    std::snprintf(
        mTalkTexts.front().data(),
        mTalkTexts.front().size(),
        "%s",
        "ここにチュートリアルの内容を入力");
}

bool StageEnemyCreationForm::Draw()
{
    if (ImGui::TreeNode("敵追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、敵を追加できません");
            ImGui::TreePop();
            return false;
        }

        StageCreationFormWidgets::DrawPlanetCombo(mContext, "敵の追加先惑星", mSelectedPlanetIndex);

        const std::uint64_t currentPresetRevision =
            EnemyPresetRepository::GetRevision();
        if (!mEnemyPresetsLoaded ||
            mLoadedEnemyPresetRevision != currentPresetRevision) {
            mEnemyPresetsLoaded = true;
            mLoadedEnemyPresetRevision = currentPresetRevision;
            EnemyPresetRepository::Load(
                "../assets/data/actor/enemies.yaml",
                mEnemyPresets,
                mEnemyPresetLoadError);
        }
        if (mEnemyPresets.empty()) {
            ImGui::TextWrapped(
                "敵プリセットを読み込めません: %s",
                mEnemyPresetLoadError.c_str());
            ImGui::TreePop();
            return false;
        }

        mSelectedEnemyTypeIndex = std::clamp(
            mSelectedEnemyTypeIndex,
            0,
            static_cast<int>(mEnemyPresets.size()) - 1);
        const EnemyPresetDefinition& selectedEnemyPreset =
            mEnemyPresets[mSelectedEnemyTypeIndex];
        if (ImGui::BeginCombo(
                "敵プリセット",
                selectedEnemyPreset.displayName.c_str())) {
            for (std::size_t presetIndex = 0;
                 presetIndex < mEnemyPresets.size();
                 ++presetIndex) {
                const bool isSelected =
                    static_cast<int>(presetIndex) ==
                    mSelectedEnemyTypeIndex;
                const std::string label =
                    mEnemyPresets[presetIndex].displayName +
                    " (" + mEnemyPresets[presetIndex].id + ")";
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    mSelectedEnemyTypeIndex =
                        static_cast<int>(presetIndex);
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        const bool canAddEnemy = mSelectedPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            const std::string enemyType =
                mEnemyPresets[mSelectedEnemyTypeIndex].id;
            mPlacementController.BeginPlacement(
                "敵",
                mSelectedPlanetIndex,
                [this, enemyType](int planetIndex, const StageActorPlacement& placement) {
                    return mCreateService.AddEnemy(enemyType, planetIndex, &placement);
                });
        }

        if (!canAddEnemy) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }
    return true;
}

void StageNPCCreationForm::Draw()
{
    if (ImGui::TreeNode("NPC追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、NPCを追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "NPCの追加先惑星", mSelectedPlanetIndex);

            ImGui::InputTextWithHint(
                "##npcModelSearch",
                "NPCモデル名を検索",
                mModelSearch.data(),
                mModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText = ToLower(mModelSearch.data());

            ImGui::BeginChild("NPCModelAssetPicker", ImVec2(0.0f, 180.0f), true);
            for (const std::string& modelPath : modelAssets) {
                if (!searchText.empty() &&
                    ToLower(modelPath).find(searchText) == std::string::npos) {
                    continue;
                }

                const bool selected = modelPath == mSelectedModel;
                if (ImGui::Selectable(modelPath.c_str(), selected)) {
                    mSelectedModel = modelPath;
                }
            }
            ImGui::EndChild();

            ImGui::Button(
                "モデルアセットをここへドロップ##newNPCModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedNPCModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedNPCModelPath)) {
                mSelectedModel = droppedNPCModelPath;
            }

            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedModel.empty() ? "未選択" : mSelectedModel.c_str());
            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");

            ImGui::InputText("NPC名", mName.data(), mName.size());
            ImGui::DragFloat(
                "初期スケール",
                &mScale,
                0.01f,
                0.01f,
                30.0f,
                "%.2f");
            ImGui::DragFloat(
                "会話できる距離",
                &mTalkRadius,
                0.05f,
                0.1f,
                20.0f,
                "%.2f");

            ImGui::SeparatorText("会話内容");
            for (std::size_t talkIndex = 0;
                 talkIndex < mTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "会話 " + std::to_string(talkIndex + 1) +
                    "##newNPCTalk" + std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mTalkTexts[talkIndex].data(),
                    mTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("この会話を削除##newNPCTalkDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mTalkTexts.erase(
                        mTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(talkIndex));
                    break;
                }
            }

            if (ImGui::Button("会話を追加##newNPC")) {
                mTalkTexts.emplace_back();
            }

            const bool canAddNPC =
                mSelectedPlanetIndex >= 0 &&
                !mSelectedModel.empty();

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星とモデルを選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(mTalkTexts.size());
                for (const auto& talkText : mTalkTexts) {
                    talkTexts.emplace_back(talkText.data());
                }

                const std::string modelPath = mSelectedModel;
                const std::string name = mName.data();
                const float talkRadius = mTalkRadius;
                const float scale = mScale;
                mPlacementController.BeginPlacement(
                    "NPC",
                    mSelectedPlanetIndex,
                    [this, modelPath, name, talkTexts, talkRadius, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddNPC(
                            modelPath, planetIndex, name, talkTexts, talkRadius, scale, &placement);
                    });
                mStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            if (!mStatus.empty()) {
                ImGui::TextUnformatted(mStatus.c_str());
            }

            ImGui::TreePop();
        }
    }
}

void StageTutorialTriggerCreationForm::Draw()
{
    if (ImGui::TreeNode("チュートリアルトリガー追加")) {
        const auto& planets =
            mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::TextUnformatted(
                "追加先の惑星がありません");
        } else {
        StageCreationFormWidgets::DrawPlanetCombo(mContext, 
                "追加先の惑星##tutorialTrigger",
                mSelectedPlanetIndex);

            ImGui::InputTextWithHint(
                "##tutorialTriggerModelSearch",
                "箱型モデルを検索",
                mModelSearch.data(),
                mModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText =
                ToLower(
                    mModelSearch.data());
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
                    mSelectedModel;
                if (ImGui::Selectable(
                        modelPath.c_str(),
                        selected)) {
                    mSelectedModel =
                        modelPath;
                }
            }
            ImGui::EndChild();
            ImGui::Button(
                "モデルアセットをここへドロップ##newTutorialTriggerModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedTutorialTriggerModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedTutorialTriggerModelPath)) {
                mSelectedModel =
                    droppedTutorialTriggerModelPath;
            }
            ImGui::Text(
                "選択中のモデル: %s",
                mSelectedModel.empty()
                    ? "未選択"
                    : mSelectedModel.c_str());

            ImGui::DragFloat3(
                "初期スケール##tutorialTrigger",
                &mScale.x,
                0.05f,
                0.01f,
                100.0f,
                "%.2f");

            ImGui::SeparatorText("チュートリアル内容");
            for (std::size_t talkIndex = 0;
                 talkIndex <
                 mTalkTexts.size();
                 ++talkIndex) {
                const std::string label =
                    "ページ " +
                    std::to_string(talkIndex + 1) +
                    "##newTutorialTriggerTalk" +
                    std::to_string(talkIndex);
                ImGui::InputTextMultiline(
                    label.c_str(),
                    mTalkTexts[talkIndex].data(),
                    mTalkTexts[talkIndex].size(),
                    ImVec2(-1.0f, 70.0f));

                if (mTalkTexts.size() > 1 &&
                    ImGui::Button(
                        ("このページを削除##newTutorialTriggerDelete" +
                         std::to_string(talkIndex))
                            .c_str())) {
                    mTalkTexts.erase(
                        mTalkTexts.begin() +
                        static_cast<std::ptrdiff_t>(
                            talkIndex));
                    break;
                }
            }

            if (ImGui::Button(
                    "ページを追加##newTutorialTrigger")) {
                mTalkTexts.emplace_back();
            }

            const bool canAdd =
                mSelectedPlanetIndex >= 0 &&
                !mSelectedModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button(
                    "チュートリアルトリガーを追加")) {
                std::vector<std::string> talkTexts;
                talkTexts.reserve(
                    mTalkTexts.size());
                for (const auto& talkText :
                     mTalkTexts) {
                    talkTexts.emplace_back(
                        talkText.data());
                }

                const std::string modelPath = mSelectedModel;
                const glm::vec3 scale = mScale;
                mPlacementController.BeginPlacement(
                    "チュートリアルトリガー",
                    mSelectedPlanetIndex,
                    [this, modelPath, talkTexts, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddTutorialTrigger(
                            planetIndex, modelPath, talkTexts, scale, &placement);
                    });
                mStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mStatus.empty()) {
                ImGui::TextUnformatted(
                    mStatus.c_str());
            }
            ImGui::TextDisabled(
                "箱型モデルを使うと、モデルの位置・回転・スケールと反応範囲が一致します。");
            ImGui::TextDisabled(
                "ゲーム中は見えず、衝突しません。内部に入ると一度だけ開始します。");
        }

        ImGui::TreePop();
    }
}

StageJewelItemCreationForm::StageJewelItemCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StageHazardActorCreationForm::StageHazardActorCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StageBoatArrivalPointCreationForm::StageBoatArrivalPointCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

void StageJewelItemCreationForm::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("ジュエルアイテム追加")) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、ジュエルアイテムを追加できません");
        ImGui::TreePop();
        return;
    }

    StageCreationFormWidgets::DrawPlanetCombo(mContext, 
        "追加先の惑星##jewelItem",
        mSelectedPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newJewelItemModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedModel = droppedModelPath;
    }
    ImGui::TextWrapped("モデル: %s", mSelectedModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newJewelItemTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedTexture.c_str());
    ImGui::DragFloat3(
        "初期スケール##jewelItem",
        &mScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedPlanetIndex >= 0 &&
        !mSelectedModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ジュエルアイテムを追加")) {
        const std::string modelPath = mSelectedModel;
        const std::string texturePath = mSelectedTexture;
        const glm::vec3 scale = mScale;
        mPlacementController.BeginPlacement(
            "ジュエルアイテム",
            mSelectedPlanetIndex,
            [this, modelPath, texturePath, scale](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddJewelItem(
                    planetIndex,
                    modelPath,
                    texturePath,
                    scale,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }
    ImGui::TextDisabled(
        "追加解除まで、ゲーム画面をクリックするたびに配置できます。");
    ImGui::TreePop();
}

void StageHazardActorCreationForm::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("危険アクター追加")) {
        return;
    }

    const auto& planets =
        mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、危険アクターを追加できません");
        ImGui::TreePop();
        return;
    }

    StageCreationFormWidgets::DrawPlanetCombo(mContext, 
        "追加先の惑星##hazardActor",
        mSelectedPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newHazardActorModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedModel = droppedModelPath;
    }
    ImGui::TextWrapped(
        "モデル: %s",
        mSelectedModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newHazardActorTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedTexture.empty()
            ? "モデル標準"
            : mSelectedTexture.c_str());

    ImGui::DragFloat3(
        "初期スケール##hazardActor",
        &mScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");
    ImGui::DragFloat(
        "基準判定半径（スケール1）##hazardActor",
        &mTriggerRadius,
        0.01f,
        0.01f,
        100.0f,
        "%.2f");
    ImGui::TextDisabled(
        "判定はアクターの各軸スケールと回転に追従します。");
    ImGui::DragFloat(
        "ダメージ##hazardActor",
        &mDamage,
        0.5f,
        0.0f,
        1000.0f,
        "%.1f");
    ImGui::DragFloat(
        "再ダメージ間隔（秒）##hazardActor",
        &mDamageIntervalSeconds,
        0.05f,
        0.0f,
        30.0f,
        "%.2f");
    ImGui::TextDisabled(
        "接触時と攻撃を当てた時に同じダメージ・ノックバックを与えます");

    const bool canAdd =
        mSelectedPlanetIndex >= 0 &&
        !mSelectedModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("危険アクターを追加")) {
        const std::string modelPath =
            mSelectedModel;
        const std::string texturePath =
            mSelectedTexture;
        const glm::vec3 scale = mScale;
        const float triggerRadius =
            mTriggerRadius;
        const float damage = mDamage;
        const float damageIntervalSeconds =
            mDamageIntervalSeconds;
        mPlacementController.BeginPlacement(
            "危険アクター",
            mSelectedPlanetIndex,
            [this,
             modelPath,
             texturePath,
             scale,
             triggerRadius,
             damage,
             damageIntervalSeconds](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddHazardActor(
                    planetIndex,
                    modelPath,
                    texturePath,
                    scale,
                    triggerRadius,
                    damage,
                    damageIntervalSeconds,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }
    ImGui::TextDisabled(
        "追加解除まで、ゲーム画面をクリックするたびに配置できます");
    ImGui::TreePop();
}

void StageBoatArrivalPointCreationForm::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage() ||
        !mContext.assetCatalog ||
        !ImGui::TreeNode("ロケット到着ポイント追加")) {
        return;
    }

    const auto& planets =
        mContext.game->GetCurrentStage()->GetPlanets();
    if (planets.empty()) {
        ImGui::TextUnformatted(
            "惑星が存在しないため、ロケット到着ポイントを追加できません");
        ImGui::TreePop();
        return;
    }

    StageCreationFormWidgets::DrawPlanetCombo(mContext, 
        "追加先の惑星##boatArrivalPoint",
        mSelectedPlanetIndex);

    ImGui::InputTextWithHint(
        "##boatArrivalPointModelSearch",
        "モデル名を検索",
        mModelSearch.data(),
        mModelSearch.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string searchText =
        ToLower(mModelSearch.data());

    ImGui::BeginChild(
        "BoatArrivalPointModelAssetPicker",
        ImVec2(0.0f, 180.0f),
        true);
    for (const std::string& modelPath : modelAssets) {
        if (!searchText.empty() &&
            ToLower(modelPath).find(searchText) ==
                std::string::npos) {
            continue;
        }

        const bool isSelected =
            modelPath == mSelectedModel;
        if (ImGui::Selectable(
                modelPath.c_str(),
                isSelected)) {
            mSelectedModel = modelPath;
        }
    }
    ImGui::EndChild();

    ImGui::Button(
        "モデルアセットをここへドロップ##newBoatArrivalPointModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedModel = droppedModelPath;
    }

    ImGui::TextWrapped(
        "選択中: %s",
        mSelectedModel.empty()
            ? "未選択"
            : mSelectedModel.c_str());
    ImGui::DragFloat3(
        "初期スケール##boatArrivalPoint",
        &mScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedPlanetIndex >= 0 &&
        !mSelectedModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ロケット到着ポイントを追加")) {
        const std::string modelPath =
            mSelectedModel;
        const glm::vec3 scale =
            mScale;
        mPlacementController.BeginPlacement(
            "ロケット到着ポイント",
            mSelectedPlanetIndex,
            [this, modelPath, scale](
                int planetIndex,
                const StageActorPlacement& placement) {
                return mCreateService.AddBoatArrivalPoint(
                    planetIndex,
                    modelPath,
                    scale,
                    &placement);
            });
    }

    if (!canAdd) {
        ImGui::EndDisabled();
    }

    ImGui::TextDisabled(
        "追加後は一覧・クリック選択・ギズモ・複製・削除を利用できます。");
    ImGui::TreePop();
}
