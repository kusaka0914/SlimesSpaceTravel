#pragma once

#include "editor/stage/StageEditOperation.h"

#include <cstddef>
#include <string>
#include <vector>

// Undo/Redo用のYAMLテキストを保持する。ファイル復元とステージの再読み込みは呼び出し元が行う。
class StageEditHistory {
public:
    explicit StageEditHistory(std::size_t maximumUndoCount = 20);

    // 同じステージの再読込では履歴を維持し、別パスへの切替時はUndo/Redoを解除する。
    void BindToStage(const std::string& stagePath);

    // スナップショットを追加し、Redo履歴を破棄する。上限を超えたUndo履歴は古いものから除く。
    void PushUndoSnapshot(const std::string& yamlText);

    // 保存とRuntime反映の成功後に呼ぶ。同じ操作の連続した変更だけを1履歴へまとめる。
    void RecordSavedChange(const std::string& before, const std::string& after, StageEditGrouping grouping);

    // 履歴がなければ nullptr。返却ポインタは履歴の変更で無効になり得る。
    const std::string* FindUndoSnapshot() const;

    // Redo履歴を借用する。寿命はUndo履歴と同じで、履歴がなければ nullptr。
    const std::string* FindRedoSnapshot() const;

    // ファイル復元に成功してから呼ぶ。復元前のテキストを反対側の履歴へ移す。
    void CommitUndo(const std::string& currentYamlText);

    // Redoのファイル復元成功後に呼び、復元前のテキストをUndo履歴へ移す。
    void CommitRedo(const std::string& currentYamlText);
    void Clear();

    std::size_t GetUndoCount() const;
    std::size_t GetRedoCount() const;

private:
    std::size_t mMaximumUndoCount;
    std::vector<std::string> mUndoSnapshots;
    std::vector<std::string> mRedoSnapshots;
    std::string mLastGroupedYamlText;
    std::string mStagePath;
};
