#pragma once

#include "gfx/debug/stage/StageEditorTypes.h"

#include <glm/glm.hpp>
#include <vector>

// 1回の選択ドラッグで使用する対象と移動量を保持する。Actor自体は所有しない。
class UGCSelectionDragState {
public:
    void Reset();

    bool isDragging = false;
    bool isMovingPlatformDestination = false;
    bool hasMoved = false;
    bool hasSavedEdit = false;
    glm::ivec3 dragStartCell{0};
    glm::ivec3 appliedHorizontalCellDelta{0};
    glm::vec3 initialCenter{0.0f};

    // ワールド座標の累積移動量。savedDeltaはドラッグ中のレイヤー変更で既に保存した移動先の差分。
    glm::vec3 appliedDelta{0.0f};
    glm::vec3 savedDelta{0.0f};
    std::vector<StageActorRef> actorRefs;
};
