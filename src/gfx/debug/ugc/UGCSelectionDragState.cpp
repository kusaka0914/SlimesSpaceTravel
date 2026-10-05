#include "gfx/debug/ugc/UGCSelectionDragState.h"

void UGCSelectionDragState::Reset()
{
    isDragging = false;
    isMovingPlatformDestination = false;
    hasMoved = false;
    dragStartCell = glm::ivec3(0);
    appliedHorizontalCellDelta = glm::ivec3(0);
    initialCenter = glm::vec3(0.0f);
    appliedDelta = glm::vec3(0.0f);
    savedDelta = glm::vec3(0.0f);
    actorRefs.clear();
}
