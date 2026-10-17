#pragma once

#include <filesystem>
#include <set>
#include <string>

// クリア・会話・操作方式の進行状態を保持し、YAMLで永続化する。
class StageProgressSystem {
public:
    StageProgressSystem();
    explicit StageProgressSystem(std::filesystem::path savePath);
    StageProgressSystem(std::filesystem::path savePath, bool isPersistenceEnabled);

    // メモリ上の進行を初期化して読み込む。保存無効・ファイルを開けない場合は初期状態のままtrue。
    bool Load();

    // 現在の進行を保存する。保存無効なら書き込まずにtrueを返す。
    bool Save() const;

    // メモリ上の進行だけを初期化する。保存ファイルは変更しない。
    void Reset();

    bool IsStageCleared(int stageNum) const;

    // クリアを記録して保存を試みる。戻り値は状態変更の有無を表し、保存成功の有無ではない。
    bool MarkStageCleared(int stageNum);

    // クリア状態を変更して保存を試みる。負の番号・変更なしはfalse。保存結果は戻り値に含めない。
    bool SetStageCleared(int stageNum, bool isCleared);
    bool HasShownConversation(const std::string& conversationId) const;

    // 空でないIDを既読にして保存を試みる。新規登録ならtrue。保存結果は戻り値に含めない。
    bool MarkConversationShown(const std::string& conversationId);

    bool HasCompletedEndingRoll() const { return mHasCompletedEndingRoll; }

    // 完走状態を更新して保存を試みる。状態が変わればtrue。保存結果は戻り値に含めない。
    bool SetEndingRollCompleted(bool completed = true);

    bool HasSelectedPlayerControlStyle() const { return mHasSelectedPlayerControlStyle; }
    bool IsAssistControlStyleSelected() const { return mIsAssistControlStyleSelected; }

    // 操作方式を選択済みとして保存を試みる。状態が変わればtrue。保存結果は戻り値に含めない。
    bool SetSelectedPlayerControlStyle(bool isAssistControlStyle);

private:
    std::filesystem::path mSavePath;
    std::set<int> mClearedStages;
    std::set<std::string> mShownConversationIds;
    bool mHasCompletedEndingRoll = false;
    bool mHasSelectedPlayerControlStyle = false;
    bool mIsAssistControlStyleSelected = false;
    bool mIsPersistenceEnabled = true;
};
