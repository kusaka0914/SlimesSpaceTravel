#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/stage/StageEditHistory.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include <glm/glm.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

// 削除・複製・Undo/RedoをYAMLに反映し、成功時にステージを再読み込みする。
class StageEditCommandController {
public:
    StageEditCommandController(DebugEditorContext& context, StageSelectionController& selectionController);

    // 現在のYAMLをUndo履歴へ記録する。UGCではクリア検証済み状態も解除して保存する。
    void PushUndo();

    // 復元できた場合は履歴を進め、選択を解除してステージを再読み込みする。失敗時は false。
    bool RestoreUndo();

    // Undoと同じ復元手順でRedo履歴を適用する。復元できない場合は履歴を進めず false。
    bool RestoreRedo();
    void ClearHistory();

    // 指定キーに対応するActorを削除し、スイッチの参照添字を更新する。惑星は対象外。
    bool DeleteSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    // 惑星と所属Actorを削除し、Playerの所属先と残る惑星の添字を更新する。最後の惑星は削除できない。
    bool DeletePlanet(int planetIndex);

    // 所属Actorを隣の惑星へ移して惑星だけを削除する。Actorのワールド位置を維持し、最後の惑星は残す。
    bool DeletePlanetOnly(int planetIndex);

    // 選択Actorを複製し、選択した惑星同士の参照先も複製先へ付け替える。成功時は複製先を選択する。
    bool DuplicateSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    // 配置UIを開く要求を返し、記録した要求を消費する。
    bool ConsumeRequestOpenPlacement();

private:
    void OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const;

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;

    StageEditHistory mEditHistory;

    bool mRequestOpenPlacement = false;
};
