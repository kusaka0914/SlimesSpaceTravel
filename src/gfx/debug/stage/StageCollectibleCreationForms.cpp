#include "gfx/debug/stage/StageCollectibleCreationForms.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageCreationFormWidgets.h"
#include "imgui.h"

#include <string>

StageCrystalCreationForm::StageCrystalCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StageBoatPartsCreationForm::StageBoatPartsCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

StageStarCreationForm::StageStarCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

void StageCrystalCreationForm::Draw()
{
    if (ImGui::TreeNode("クリスタル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、クリスタルを追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "クリスタルの追加先惑星", mSelectedPlanetIndex);

            const char* crystalTypeLabels[] = {"小さいクリスタル", "大きいクリスタル"};
            const char* crystalTypes[] = {"little", "big"};

            ImGui::Combo("クリスタルタイプ", &mSelectedTypeIndex, crystalTypeLabels,
                         IM_ARRAYSIZE(crystalTypeLabels));

            const bool canAddCrystal = mSelectedPlanetIndex >= 0;

            if (!canAddCrystal) {
                ImGui::Text("クリスタルを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("クリスタルを追加")) {
                const std::string crystalType = crystalTypes[mSelectedTypeIndex];
                mPlacementController.BeginPlacement(
                    "クリスタル",
                    mSelectedPlanetIndex,
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
}

void StageBoatPartsCreationForm::Draw()
{
    if (ImGui::TreeNode("ボートパーツ追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートパーツを追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "ボートパーツの追加先惑星", mSelectedPlanetIndex);

            const char* boatPartsTypeLabels[] = {"パーツ1", "パーツ2", "パーツ3", "パーツ4", "パーツ5"};
            const char* boatPartsTypes[] = {"parts1", "parts2", "parts3", "parts4", "parts5"};

            ImGui::Combo("ボートパーツタイプ", &mSelectedTypeIndex, boatPartsTypeLabels,
                         IM_ARRAYSIZE(boatPartsTypeLabels));

            const bool canAddBoatParts = mSelectedPlanetIndex >= 0;

            if (!canAddBoatParts) {
                ImGui::Text("ボートパーツを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートパーツを追加")) {
                const std::string boatPartsType = boatPartsTypes[mSelectedTypeIndex];
                mPlacementController.BeginPlacement(
                    "ボートパーツ",
                    mSelectedPlanetIndex,
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
}

void StageStarCreationForm::Draw()
{
    if (ImGui::TreeNode("星追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、星を追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "星の追加先惑星", mSelectedPlanetIndex);

            const bool canAddStar = mSelectedPlanetIndex >= 0;

            if (!canAddStar) {
                ImGui::Text("星を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("星を追加")) {
                mPlacementController.BeginPlacement(
                    "星",
                    mSelectedPlanetIndex,
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
