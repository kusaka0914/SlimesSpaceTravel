#pragma once

#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageEditHistory.h"
#include "editor/stage/StageEditTransaction.h"

#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <unordered_set>
#include <vector>
#include <yaml-cpp/yaml.h>

class StageSelectionController;

// 削除・複製・Undo/RedoをYAMLに反映し、成功時にステージを再読み込みする。
class StageEditCommandController {
public:
    StageEditCommandController(DebugEditorContext& context, StageSelectionController& selectionController);

    // 操作の保存・Runtime反映が成功した場合だけUndoを確定する。失敗時は必要に応じて旧状態へ戻す。
    // 復旧用の再読込でActorが置き換わり得るため、呼出後は古いActorポインタを使わないこと。
    bool ExecuteEdit(const StageEditOperation& operation, StageEditGrouping grouping = StageEditGrouping::NewOperation,
                     StageEditFailureRecovery recovery = StageEditFailureRecovery::RestoreChangedFile);
    const std::string& GetLastError() const { return mLastError; }

    // 保存・Runtime反映に成功した編集の、変更前YAMLを記録する。保存や再読込は行わない。
    void RecordSavedEdit(const std::string& previousYamlText);

    // 復元できた場合は履歴を進め、選択を解除してステージを再読み込みする。失敗時は false。
    bool RestoreUndo();

    // Undoと同じ復元手順でRedo履歴を適用する。復元できない場合は履歴を進めず false。
    bool RestoreRedo();
    void ClearHistory();

    // 履歴の対象パスを現在のステージへ同期する。同じパスの編集履歴は維持する。
    void SynchronizeStage();

    // 指定キーに対応するActorを削除し、スイッチの参照添字を更新する。惑星は対象外。
    bool DeleteSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    // 惑星と所属Actorを削除し、Playerの所属先と残る惑星の添字を更新する。最後の惑星は削除できない。
    bool DeletePlanet(int planetIndex);

    // 所属Actorを隣の惑星へ移して惑星だけを削除する。Actorのワールド位置を維持し、最後の惑星は残す。
    bool DeletePlanetOnly(int planetIndex);

    // 選択Actorを複製し、選択した惑星同士の参照先も複製先へ付け替える。成功時は複製先を選択する。
    // 移動床の経路も配置と同じだけずらす。惑星と一緒に複製する所属物には変位を重ねない。
    bool DuplicateSelectedKeys(const std::unordered_set<std::string>& selectedKeys);

    // 配置UIを開く要求を返し、記録した要求を消費する。
    bool ConsumeRequestOpenPlacement();

private:
    bool ApplyStageText(const std::string& yamlText, const std::string& previousYamlText);
    bool ApplyStageChange(const std::string& previousYamlText, const std::function<bool()>& saveChanges);
    bool CommitStageEdit(YAML::Node config, bool preserveRuntimePlanetCenters = true);

    void OffsetDuplicatedActorNode(YAML::Node actorNode, const glm::vec3& offset) const;

    DebugEditorContext& mContext;
    StageSelectionController& mSelectionController;

    StageEditHistory mEditHistory;
    bool mIsExecutingEdit = false;
    std::string mLastError;

    bool mRequestOpenPlacement = false;
};
