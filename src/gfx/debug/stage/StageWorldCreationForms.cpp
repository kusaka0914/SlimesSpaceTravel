#include "gfx/debug/stage/StageWorldCreationForms.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageCreationFormWidgets.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
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

StageObjectCreationForm::StageObjectCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StagePlanetCreationForm::StagePlanetCreationForm(
    StageActorCreateService& actorCreateService)
    : mCreateService(actorCreateService)
{
}

StagePlatformCreationForm::StagePlatformCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

void StageObjectCreationForm::Draw()
{
    if (ImGui::TreeNode("汎用モデル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();
        if (planets.empty()) {
            ImGui::TextUnformatted("惑星が存在しないため、モデルを追加できません");
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "追加先の惑星##stageObject", mSelectedPlanetIndex);
            ImGui::InputTextWithHint(
                "##stageObjectSearch",
                "モデル名を検索",
                mModelSearch.data(),
                mModelSearch.size());

            const std::vector<std::string>& modelAssets =
                mContext.assetCatalog->GetPaths(EditorAssetType::Model);
            const std::string searchText = ToLower(mModelSearch.data());

            ImGui::BeginChild("StageObjectAssetPicker", ImVec2(0.0f, 180.0f), true);
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
                "モデルアセットをここへドロップ##newStageObjectModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedStageObjectModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedStageObjectModelPath)) {
                mSelectedModel =
                    droppedStageObjectModelPath;
            }

            ImGui::Text(
                "選択中: %s",
                mSelectedModel.empty()
                    ? "未選択"
                    : mSelectedModel.c_str());
            ImGui::Checkbox("モデル形状の当たり判定を作る", &mIsCollisionEnabled);

            const bool canAdd =
                mSelectedPlanetIndex >= 0 &&
                !mSelectedModel.empty();
            if (!canAdd) {
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("選択したモデルをステージに追加")) {
                const std::string modelPath = mSelectedModel;
                const bool collisionEnabled = mIsCollisionEnabled;
                mPlacementController.BeginPlacement(
                    "ステージモデル",
                    mSelectedPlanetIndex,
                    [this, modelPath, collisionEnabled](
                        int planetIndex,
                        const StageActorPlacement& placement) {
                        return mCreateService.AddStageObject(
                            planetIndex, modelPath, collisionEnabled, &placement);
                    });
                mStatus = "ゲーム画面をクリックして配置してください";
            }

            if (!canAdd) {
                ImGui::EndDisabled();
            }

            if (!mStatus.empty()) {
                ImGui::SameLine();
                ImGui::TextUnformatted(mStatus.c_str());
            }

            ImGui::TextDisabled(
                "assets/models 内の対応モデルは自動的にこの一覧へ反映されます。");
        }
        ImGui::TreePop();
    }
}

void StagePlanetCreationForm::Draw()
{
    if (ImGui::TreeNode("惑星追加")) {
        const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
        const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

        if (ImGui::Combo(
                "惑星モデル",
                &mSelectedModelIndex,
                planetModelLabels,
                IM_ARRAYSIZE(planetModelLabels))) {
            mSelectedModelPath =
                planetModels[mSelectedModelIndex];
        }

        ImGui::Button(
            "モデルアセットをここへドロップ##newPlanetModel",
            ImVec2(-1.0f, 0.0f));
        std::string droppedPlanetModelPath;
        if (EditorAssetDragDrop::AcceptPath(
                EditorAssetType::Model,
                droppedPlanetModelPath)) {
            mSelectedModelPath = droppedPlanetModelPath;
        }
        ImGui::TextWrapped(
            "選択中: %s",
            mSelectedModelPath.c_str());

        if (ImGui::Button("惑星を追加")) {
            mCreateService.AddPlanet(mSelectedModelPath);
        }

        ImGui::TreePop();
    }
}

void StagePlatformCreationForm::Draw()
{
    if (ImGui::TreeNode("足場追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、足場を追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "追加先の惑星##platform", mSelectedPlanetIndex);

            const char* platformModelLabels[] = {"通常足場", "カーブ足場", "細い足場"};
            const char* platformModels[] = {"platform.obj", "curvePlatform.obj", "platform_thin.obj"};

            if (ImGui::Combo(
                    "モデル##platform",
                    &mSelectedModelIndex,
                    platformModelLabels,
                    IM_ARRAYSIZE(platformModelLabels))) {
                mSelectedModelPath =
                    platformModels[mSelectedModelIndex];
            }
            ImGui::Button(
                "モデルアセットをここへドロップ##newPlatformModel",
                ImVec2(-1.0f, 0.0f));
            std::string droppedPlatformModelPath;
            if (EditorAssetDragDrop::AcceptPath(
                    EditorAssetType::Model,
                    droppedPlatformModelPath)) {
                mSelectedModelPath =
                    droppedPlatformModelPath;
            }
            ImGui::TextWrapped(
                "選択中: %s",
                mSelectedModelPath.c_str());

            ImGui::SliderFloat("スケールX##platform", &mScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &mScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &mScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = mSelectedPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                const std::string modelPath = mSelectedModelPath;
                const glm::vec3 scale = mScale;
                mPlacementController.BeginPlacement(
                    "足場",
                    mSelectedPlanetIndex,
                    [this, modelPath, scale](int planetIndex, const StageActorPlacement& placement) {
                        return mCreateService.AddPlatform(planetIndex, modelPath, scale, &placement);
                    });
            }

            if (ImGui::Button("乗ると動く足場を追加")) {
                const std::string modelPath = mSelectedModelPath;
                const glm::vec3 scale = mScale;
                mPlacementController.BeginPlacement(
                    "乗ると動く足場",
                    mSelectedPlanetIndex,
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
}
