#include "gfx/debug/ugc/UGCSelectionDragState.h"

void UGCSelectionDragState::Reset()
{
    isDragging = false;
    isMovingPlatformDestination = false;
    hasMoved = false;
    planePoint = glm::vec3(0.0f);
    planeNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    offset = glm::vec3(0.0f);
    initialCenter = glm::vec3(0.0f);
    appliedDelta = glm::vec3(0.0f);
    savedDelta = glm::vec3(0.0f);
    actorRefs.clear();
}
