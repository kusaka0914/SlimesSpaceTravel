#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <functional>
#include <string>

class Actor;
class Player;

// Playerの初期配置保存と、選択Actor付近へのデバッグ移動を提供する。
class StagePlayerEditor {
public:
    using SavedEditCallback = std::function<void(const std::string&)>;

    explicit StagePlayerEditor(DebugEditorContext& context, SavedEditCallback recordSavedEdit = {});

    // Playerの現在の配置を初期配置として保存するUIを描画する。
    void DrawSpawnEditor();

    // 選択Actorの近くへ操作中Playerを移動するUIを描画する。初期配置は保存しない。
    void DrawDebugMover(Actor* selectedActor);

private:
    bool SaveSpawnFromCurrentTransform(Player* player);
    bool MoveControlledPlayerToSelectedActor(Actor* selectedActor);

    DebugEditorContext& mContext;
    SavedEditCallback mRecordSavedEdit;
    int mSelectedSpawnPlayerIndex = 0;
    std::string mPlayerSpawnStatus;
    std::string mPlayerDebugMoveStatus;
};
