#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>
#include <vector>

class UGCSelectionDragState {
public:
    void Reset();

    bool isDragging = false;
    bool isMovingPlatformDestination = false;
    bool hasMoved = false;
    glm::ivec3 dragStartCell{0};
    glm::ivec3 appliedHorizontalCellDelta{0};
    glm::vec3 initialCenter{0.0f};
    glm::vec3 appliedDelta{0.0f};
    glm::vec3 savedDelta{0.0f};
    std::vector<StageActorRef> actorRefs;
};
