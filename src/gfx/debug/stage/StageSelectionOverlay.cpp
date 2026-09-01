#include "gfx/debug/stage/StageSelectionOverlay.h"

#include "gfx/debug/stage/StageSelectionController.h"
#include "imgui.h"

void DrawStageSelectionOverlay(
    const StageSelectionController& selectionController)
{
    const std::optional<StageSelectionScreenRect> selectionRect =
        selectionController.FindActiveBoxSelectionScreenRect();
    if (!selectionRect) {
        return;
    }

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    drawList->AddRectFilled(
        selectionRect->minimum,
        selectionRect->maximum,
        IM_COL32(255, 150, 0, 45));
    drawList->AddRect(
        selectionRect->minimum,
        selectionRect->maximum,
        IM_COL32(255, 150, 0, 220),
        0.0f,
        0,
        2.0f);
}
