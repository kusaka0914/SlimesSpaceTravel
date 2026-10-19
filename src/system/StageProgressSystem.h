#pragma once

#include "state/ProgressUpdateResult.h"

#include <filesystem>
#include <set>
#include <string>

// クリア・会話・操作方式の進行状態を保持し、YAMLで永続化する。
class StageProgressSystem {
public:
    StageProgressSystem();
    explicit StageProgressSystem(std::filesystem::path savePath);
    StageProgressSystem(std::filesystem::path savePath, bool isPersistenceEnabled);

    // 読込成功時だけ進行を置き換える。失敗時はfalseで現在の進行を維持する。
    // 保存無効・ファイル未作成なら初期状態にしてtrue。
    bool Load();

    // 現在の進行を保存する。保存無効なら書き込まずにtrueを返す。
    bool Save() const;

    // メモリ上の進行だけを初期化する。保存ファイルは変更しない。
    void Reset();

    bool IsStageCleared(int stageNum) const;

    // クリアを記録し、変更の有無と保存結果を返す。保存失敗でもメモリ上の進行は維持する。
    ProgressUpdateResult MarkStageCleared(int stageNum);

    // クリア状態を変更して保存を試みる。負の番号・変更なしはchanged=falseで保存しない。
    ProgressUpdateResult SetStageCleared(int stageNum, bool isCleared);
    bool HasShownConversation(const std::string& conversationId) const;

    // 空でないIDを既読にして保存を試みる。空ID・登録済みなら変更も保存もしない。
    ProgressUpdateResult MarkConversationShown(const std::string& conversationId);

    bool HasCompletedEndingRoll() const { return mHasCompletedEndingRoll; }

    // 完走状態を更新し、変更の有無と保存結果を返す。
    ProgressUpdateResult SetEndingRollCompleted(bool completed = true);

    bool HasSelectedPlayerControlStyle() const { return mHasSelectedPlayerControlStyle; }
    bool IsAssistControlStyleSelected() const { return mIsAssistControlStyleSelected; }

    // 操作方式を選択済みとして保存し、変更の有無と保存結果を返す。
    ProgressUpdateResult SetSelectedPlayerControlStyle(bool isAssistControlStyle);

private:
    ProgressUpdateResult SaveChange(bool changed) const;

    std::filesystem::path mSavePath;
    std::set<int> mClearedStages;
    std::set<std::string> mShownConversationIds;
    bool mHasCompletedEndingRoll = false;
    bool mHasSelectedPlayerControlStyle = false;
    bool mIsAssistControlStyleSelected = false;
    bool mIsPersistenceEnabled = true;
};
