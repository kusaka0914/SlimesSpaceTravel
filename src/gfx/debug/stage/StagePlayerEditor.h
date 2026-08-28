#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>

class Actor;
class Player;

class StagePlayerEditor {
public:
    explicit StagePlayerEditor(DebugEditorContext& context);

    void DrawSpawnEditor();
    void DrawDebugMover(Actor* selectedActor);

private:
    bool SaveSpawnFromCurrentTransform(Player* player);
    bool MoveControlledPlayerToSelectedActor(Actor* selectedActor);

    DebugEditorContext& mContext;
    int mSelectedSpawnPlayerIndex = 0;
    std::string mPlayerSpawnStatus;
    std::string mPlayerDebugMoveStatus;
};
