#include "gfx/debug/stage/StageCreationFormWidgets.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "gfx/debug/DebugEditorContext.h"
#include "imgui.h"

#include <string>

void StageCreationFormWidgets::DrawPlanetCombo(
    DebugEditorContext& context,
    const char* label,
    int& selectedPlanetIndex)
{
    if (!context.game || !context.game->GetCurrentStage()) {
        selectedPlanetIndex = -1;
        return;
    }

    const auto& planets = context.game->GetCurrentStage()->GetPlanets();
    if (selectedPlanetIndex >= static_cast<int>(planets.size())) {
        selectedPlanetIndex = -1;
    }

    std::string previewText = "未選択";
    if (selectedPlanetIndex >= 0) {
        previewText = "惑星 " + std::to_string(selectedPlanetIndex);
    }

    if (!ImGui::BeginCombo(label, previewText.c_str())) {
        return;
    }

    for (int planetIndex = 0;
         planetIndex < static_cast<int>(planets.size());
         ++planetIndex) {
        if (!planets[planetIndex]) {
            continue;
        }

        const std::string itemLabel =
            "惑星 " + std::to_string(planetIndex);
        const bool isSelected = selectedPlanetIndex == planetIndex;
        if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
            selectedPlanetIndex = planetIndex;
        }
        if (isSelected) {
            ImGui::SetItemDefaultFocus();
        }
    }
    ImGui::EndCombo();
}
