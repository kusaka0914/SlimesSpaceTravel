#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/enemy/EnemyPresetRepository.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "system/PhysicsSystem.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <utility>
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

void StageAddActorPanel::SetSelectionController(
    StageSelectionController* selectionController)
{
    mSelectionController = selectionController;
}

void StageAddActorPanel::SetPushUndoCallback(
    std::function<void()> pushUndoCallback)
{
    mPushUndoCallback = std::move(pushUndoCallback);
}

bool StageAddActorPanel::BeginDuplicatePlacement(
    const StageActorRef& sourceRef)
{
    if (!mContext.game ||
        !mContext.game->GetCurrentStage() ||
        !mSelectionController ||
        sourceRef.sequenceName.empty() ||
        sourceRef.yamlIndex < 0) {
        return false;
    }

    YAML::Node stageYaml;
    if (!StageYamlRepository::LoadCurrentStage(
            mContext, stageYaml)) {
        return false;
    }

    const YAML::Node sourceSequence =
        stageYaml[sourceRef.sequenceName];
    if (!sourceSequence ||
        !sourceSequence.IsSequence() ||
        sourceRef.yamlIndex >=
            static_cast<int>(sourceSequence.size())) {
        return false;
    }

    const YAML::Node sourceNode =
        sourceSequence[sourceRef.yamlIndex];
    if (!sourceNode || !sourceNode.IsMap()) {
        return false;
    }

    Actor* sourceActor = StageActorQuery::FindActorByRef(
        mContext.game->GetCurrentStage(), sourceRef);
    const int fallbackPlanetIndex =
        ResolveHitPlanetIndex(sourceActor, 0);
    const YAML::Node sourceTemplate = YAML::Clone(sourceNode);
    const std::string displayName =
        StageActorQuery::GetTypeLabel(sourceRef) +
        "（選択中の設定）";

    BeginPlacement(
        displayName,
        fallbackPlanetIndex,
        [this, sourceRef, sourceTemplate](
            int planetIndex,
            const StageActorPlacement& placement) {
            if (mPushUndoCallback) {
                mPushUndoCallback();
            }

            return mCreateService.DuplicateActorAtPlacement(
                sourceRef,
                sourceTemplate,
                planetIndex,
                placement);
        });
    return true;
}

void StageAddActorPanel::BeginPlacement(
    const std::string& displayName,
    int fallbackPlanetIndex,
    std::function<bool(int, const StageActorPlacement&)> placementCreator)
{
    mPlacementDisplayName = displayName;
    mPlacementFallbackPlanetIndex = fallbackPlanetIndex;
    mPlacementCreator = std::move(placementCreator);
    mPlacementStatus = "ゲーム画面をクリックして配置してください";
}

void StageAddActorPanel::CancelPlacement()
{
    mPlacementCreator = {};
    mPlacementDisplayName.clear();
    mPlacementFallbackPlanetIndex = -1;
    mPlacementStatus = "連続配置を終了しました";
}

int StageAddActorPanel::ResolveHitPlanetIndex(
    Actor* hitActor,
    int fallbackPlanetIndex) const
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return fallbackPlanetIndex;
    }

    Planet* hitPlanet = dynamic_cast<Planet*>(hitActor);
    if (!hitPlanet && hitActor) {
        hitPlanet = hitActor->GetCurrentPlanet();
    }
    if (!hitPlanet) {
        return fallbackPlanetIndex;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
    const auto planetIt = std::find(planets.begin(), planets.end(), hitPlanet);
    if (planetIt == planets.end()) {
        return fallbackPlanetIndex;
    }
    return static_cast<int>(std::distance(planets.begin(), planetIt));
}

void StageAddActorPanel::UpdatePlacement()
{
    if (!mPlacementCreator) {
        return;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        CancelPlacement();
        return;
    }

    if (ImGui::GetIO().WantCaptureMouse ||
        !ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return;
    }

    if (!mSelectionController || !mContext.game ||
        !mContext.game->GetPhysicsSystem()) {
        mPlacementStatus = "配置に必要なシステムを利用できません";
        return;
    }

    glm::vec3 rayFrom;
    glm::vec3 rayTo;
    if (!mSelectionController->TryCreateMouseRay(rayFrom, rayTo)) {
        return;
    }

    const std::optional<PhysicsSystem::RayHitActor> hit =
        mContext.game->GetPhysicsSystem()->RaycastStageSurface(rayFrom, rayTo);
    if (!hit) {
        mPlacementStatus = "配置できる惑星・足場・ステージモデルに当たりませんでした";
        return;
    }

    const int planetIndex =
        ResolveHitPlanetIndex(hit->actor, mPlacementFallbackPlanetIndex);
    if (planetIndex < 0) {
        mPlacementStatus = "クリックした面の所属惑星を特定できませんでした";
        return;
    }

    StageActorPlacement placement;
    placement.worldPosition = hit->hitPos;
    placement.surfaceNormal = hit->hitNormal;
    const bool created = mPlacementCreator(planetIndex, placement);
    mPlacementStatus = created
                           ? mPlacementDisplayName + "を配置しました。続けてクリックできます"
                           : mPlacementDisplayName + "の配置に失敗しました";
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

    DrawBoatArrivalPointCreation();
    DrawJewelItemCreation();
    DrawHazardActorCreation();

    if (mPlacementCreator) {
        ImGui::SeparatorText("連続配置中");
        ImGui::Text("配置対象: %s", mPlacementDisplayName.c_str());
        ImGui::TextWrapped("ゲーム画面をクリックするたびに追加します。");
        if (ImGui::Button("追加解除") || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            CancelPlacement();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("ESCでも解除");
        if (!mPlacementStatus.empty()) {
            ImGui::TextWrapped("%s", mPlacementStatus.c_str());
        }
        ImGui::Separator();
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

            ImGui::Button(
                "モデルアセットをここへドロップ##newStageObjectModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedStageObjectModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedStageObjectModelPath)) {
                mSelectedStageObjectModel =
                    droppedStageObjectModelPath;
            }

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
                const std::string modelPath = mSelectedStageObjectModel;
                const bool collisionEnabled = mStageObjectCollisionEnabled;
                BeginPlacement(
                    "ステージモデル",
                    mSelectedStageObjectPlanetIndex,
                    [this, modelPath, collisionEnabled](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddStageObject(
                            planetIndex, modelPath, collisionEnabled, &placement);
                    });
                mStageObjectStatus = "ゲーム画面をクリックして配置してください";
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

        if (ImGui::Combo(
                "惑星モデル",
                &mSelectedPlanetModelIndex,
                planetModelLabels,
                IM_ARRAYSIZE(planetModelLabels))) {
            mSelectedPlanetModelPath =
                planetModels[mSelectedPlanetModelIndex];
        }

        ImGui::Button(
            "モデルアセットをここへドロップ##newPlanetModel",
            ImVec2(-1.0f, 0.0f));
        std::string droppedPlanetModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedPlanetModelPath)) {
            mSelectedPlanetModelPath = droppedPlanetModelPath;
        }
        ImGui::TextWrapped(
            "選択中: %s",
            mSelectedPlanetModelPath.c_str());

        if (ImGui::Button("惑星を追加")) {
            mCreateService.AddPlanet(mSelectedPlanetModelPath);
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
            return;
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

        const bool canAddEnemy = mSelectedEnemyPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            const std::string enemyType =
                mEnemyPresets[mSelectedEnemyTypeIndex].id;
            BeginPlacement(
                "敵",
                mSelectedEnemyPlanetIndex,
                [this, enemyType](int planetIndex, const StageActorPlacement& placement) {
                    return mCreateService.AddEnemy(enemyType, planetIndex, &placement);
                });
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

            if (ImGui::Combo(
                    "モデル##platform",
                    &mSelectedPlatformModelIndex,
                    platformModelLabels,
                    IM_ARRAYSIZE(platformModelLabels))) {
                mSelectedPlatformModelPath =
                    platformModels[mSelectedPlatformModelIndex];
            }
            ImGui::Button(
                "モデルアセットをここへドロップ##newPlatformModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedPlatformModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedPlatformModelPath)) {
                mSelectedPlatformModelPath =
                    droppedPlatformModelPath;
            }
            ImGui::TextWrapped(
                "選択中: %s",
                mSelectedPlatformModelPath.c_str());

            ImGui::SliderFloat("スケールX##platform", &mPlatformScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &mPlatformScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &mPlatformScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = mSelectedPlatformPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                const std::string modelPath = mSelectedPlatformModelPath;
                const glm::vec3 scale = mPlatformScale;
                BeginPlacement(
                    "足場",
                    mSelectedPlatformPlanetIndex,
                    [this, modelPath, scale](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddPlatform(planetIndex, modelPath, scale, &placement);
                    });
            }

            if (ImGui::Button("乗ると動く足場を追加")) {
                const std::string modelPath = mSelectedPlatformModelPath;
                const glm::vec3 scale = mPlatformScale;
                BeginPlacement(
                    "乗ると動く足場",
                    mSelectedPlatformPlanetIndex,
                    [this, modelPath, scale](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddRideMovingPlatform(
                            planetIndex, modelPath, scale, &placement);
                    });
                mRideMovingPlatformStatus = "ゲーム画面をクリックして配置してください";
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
                const std::string crystalType = crystalTypes[mSelectedCrystalTypeIndex];
                BeginPlacement(
                    "クリスタル",
                    mSelectedCrystalPlanetIndex,
                    [this, crystalType](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddCrystal(crystalType, planetIndex, &placement);
                    });
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

            ImGui::Button(
                "モデルアセットをここへドロップ##newNPCModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedNPCModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedNPCModelPath)) {
                mSelectedNPCModel = droppedNPCModelPath;
            }

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

                const std::string modelPath = mSelectedNPCModel;
                const std::string name = mNPCName.data();
                const float talkRadius = mNPCTalkRadius;
                const float scale = mNPCScale;
                BeginPlacement(
                    "NPC",
                    mSelectedNPCPlanetIndex,
                    [this, modelPath, name, talkTexts, talkRadius, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddNPC(
                            modelPath, planetIndex, name, talkTexts, talkRadius, scale, &placement);
                    });
                mNPCStatus = "ゲーム画面をクリックして配置してください";
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
            ImGui::Button(
                "モデルアセットをここへドロップ##newTutorialTriggerModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedTutorialTriggerModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedTutorialTriggerModelPath)) {
                mSelectedTutorialTriggerModel =
                    droppedTutorialTriggerModelPath;
            }
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

                const std::string modelPath = mSelectedTutorialTriggerModel;
                const glm::vec3 scale = mTutorialTriggerScale;
                BeginPlacement(
                    "チュートリアルトリガー",
                    mSelectedTutorialTriggerPlanetIndex,
                    [this, modelPath, talkTexts, scale](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddTutorialTrigger(
                            planetIndex, modelPath, talkTexts, scale, &placement);
                    });
                mTutorialTriggerStatus = "ゲーム画面をクリックして配置してください";
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
                const std::string boatPartsType = boatPartsTypes[mSelectedBoatPartsTypeIndex];
                BeginPlacement(
                    "ボートパーツ",
                    mSelectedBoatPartsPlanetIndex,
                    [this, boatPartsType](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddBoatParts(boatPartsType, planetIndex, &placement);
                    });
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
                const int destinationPlanetIndex = mSelectedBoatDestPlanetIndex;
                const int destinationStage = mSelectedBoatDestStage;
                BeginPlacement(
                    "ボート",
                    mSelectedBoatStartPlanetIndex,
                    [this, destinationPlanetIndex, destinationStage](
                        int startPlanetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddBoat(
                            startPlanetIndex, destinationPlanetIndex, destinationStage, &placement);
                    });
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
                BeginPlacement(
                    "星",
                    mSelectedStarPlanetIndex,
                    [this](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddStar(planetIndex, &placement);
                    });
            }

            if (!canAddStar) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }
}

void StageAddActorPanel::DrawJewelItemCreation()
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

    DrawPlanetCombo(
        "追加先の惑星##jewelItem",
        mSelectedJewelItemPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newJewelItemModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedJewelItemModel = droppedModelPath;
    }
    ImGui::TextWrapped("モデル: %s", mSelectedJewelItemModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newJewelItemTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedJewelItemTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedJewelItemTexture.c_str());
    ImGui::DragFloat3(
        "初期スケール##jewelItem",
        &mJewelItemScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedJewelItemPlanetIndex >= 0 &&
        !mSelectedJewelItemModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ジュエルアイテムを追加")) {
        const std::string modelPath = mSelectedJewelItemModel;
        const std::string texturePath = mSelectedJewelItemTexture;
        const glm::vec3 scale = mJewelItemScale;
        BeginPlacement(
            "ジュエルアイテム",
            mSelectedJewelItemPlanetIndex,
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

void StageAddActorPanel::DrawHazardActorCreation()
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

    DrawPlanetCombo(
        "追加先の惑星##hazardActor",
        mSelectedHazardActorPlanetIndex);

    ImGui::Button(
        "モデルをここへドロップ##newHazardActorModel",
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        mSelectedHazardActorModel = droppedModelPath;
    }
    ImGui::TextWrapped(
        "モデル: %s",
        mSelectedHazardActorModel.c_str());

    ImGui::Button(
        "テクスチャをここへドロップ##newHazardActorTexture",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        mSelectedHazardActorTexture = droppedTexturePath;
    }
    ImGui::TextWrapped(
        "テクスチャ: %s",
        mSelectedHazardActorTexture.empty()
            ? "モデル標準"
            : mSelectedHazardActorTexture.c_str());

    ImGui::DragFloat3(
        "初期スケール##hazardActor",
        &mHazardActorScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");
    ImGui::DragFloat(
        "判定半径##hazardActor",
        &mHazardActorTriggerRadius,
        0.01f,
        0.01f,
        100.0f,
        "%.2f");
    ImGui::DragFloat(
        "ダメージ##hazardActor",
        &mHazardActorDamage,
        0.5f,
        0.0f,
        1000.0f,
        "%.1f");
    ImGui::DragFloat(
        "再ダメージ間隔（秒）##hazardActor",
        &mHazardActorDamageIntervalSeconds,
        0.05f,
        0.0f,
        30.0f,
        "%.2f");
    ImGui::TextDisabled(
        "接触時と攻撃を当てた時に同じダメージ・ノックバックを与えます");

    const bool canAdd =
        mSelectedHazardActorPlanetIndex >= 0 &&
        !mSelectedHazardActorModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("危険アクターを追加")) {
        const std::string modelPath =
            mSelectedHazardActorModel;
        const std::string texturePath =
            mSelectedHazardActorTexture;
        const glm::vec3 scale = mHazardActorScale;
        const float triggerRadius =
            mHazardActorTriggerRadius;
        const float damage = mHazardActorDamage;
        const float damageIntervalSeconds =
            mHazardActorDamageIntervalSeconds;
        BeginPlacement(
            "危険アクター",
            mSelectedHazardActorPlanetIndex,
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

void StageAddActorPanel::DrawBoatArrivalPointCreation()
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

    DrawPlanetCombo(
        "追加先の惑星##boatArrivalPoint",
        mSelectedBoatArrivalPointPlanetIndex);

    ImGui::InputTextWithHint(
        "##boatArrivalPointModelSearch",
        "モデル名を検索",
        mBoatArrivalPointModelSearch.data(),
        mBoatArrivalPointModelSearch.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string searchText =
        ToLower(mBoatArrivalPointModelSearch.data());

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
            modelPath == mSelectedBoatArrivalPointModel;
        if (ImGui::Selectable(
                modelPath.c_str(),
                isSelected)) {
            mSelectedBoatArrivalPointModel = modelPath;
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
        mSelectedBoatArrivalPointModel = droppedModelPath;
    }

    ImGui::TextWrapped(
        "選択中: %s",
        mSelectedBoatArrivalPointModel.empty()
            ? "未選択"
            : mSelectedBoatArrivalPointModel.c_str());
    ImGui::DragFloat3(
        "初期スケール##boatArrivalPoint",
        &mBoatArrivalPointScale.x,
        0.01f,
        0.01f,
        30.0f,
        "%.2f");

    const bool canAdd =
        mSelectedBoatArrivalPointPlanetIndex >= 0 &&
        !mSelectedBoatArrivalPointModel.empty();
    if (!canAdd) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("ロケット到着ポイントを追加")) {
        const std::string modelPath =
            mSelectedBoatArrivalPointModel;
        const glm::vec3 scale =
            mBoatArrivalPointScale;
        BeginPlacement(
            "ロケット到着ポイント",
            mSelectedBoatArrivalPointPlanetIndex,
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
