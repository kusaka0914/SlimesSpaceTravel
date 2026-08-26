#include "gfx/debug/stage/StageBoatCreationForm.h"

#include "Game.h"
#include "Stage.h"
#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageActorCreateService.h"
#include "gfx/debug/stage/StageActorPlacementController.h"
#include "gfx/debug/stage/StageCreationFormWidgets.h"
#include "imgui.h"

StageBoatCreationForm::StageBoatCreationForm(
    DebugEditorContext& context,
    StageActorCreateService& actorCreateService,
    StageActorPlacementController& placementController)
    : mContext(context),
      mCreateService(actorCreateService),
      mPlacementController(placementController)
{
}

void StageBoatCreationForm::Draw()
{
    if (ImGui::TreeNode("ボート追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートを追加できません");
            ImGui::TreePop();
        } else {
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "ボートの開始惑星", mSelectedStartPlanetIndex);
            StageCreationFormWidgets::DrawPlanetCombo(mContext, "ボートの移動先惑星", mSelectedDestinationPlanetIndex);

            ImGui::InputInt("移動先ステージ", &mSelectedDestinationStage);

            const bool canAddBoat = mSelectedStartPlanetIndex >= 0 && mSelectedDestinationPlanetIndex >= 0;

            if (!canAddBoat) {
                ImGui::Text("ボートを追加するには、開始惑星と移動先惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートを追加")) {
                const int destinationPlanetIndex = mSelectedDestinationPlanetIndex;
                const int destinationStage = mSelectedDestinationStage;
                mPlacementController.BeginPlacement(
                    "ボート",
                    mSelectedStartPlanetIndex,
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
}
