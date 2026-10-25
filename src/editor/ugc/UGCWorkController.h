#pragma once

#include "editor/ugc/UGCWorkFileService.h"

#include <functional>
#include <string>

struct DebugEditorContext;
struct UGCWorkState;

// 作品の一覧・選択・保存操作を連携し、外部のUGCWorkStateを更新する。contextとstateは所有しない。
class UGCWorkController {
public:
    // reloadSelectedWorkには、作品の読み込み・新規作成後に編集状態とステージを更新する有効な関数を渡す。
    UGCWorkController(DebugEditorContext& context, UGCWorkState& state, std::function<void()> reloadSelectedWork);

    // 保存作品の一覧を更新して選択位置を範囲内へ補正する。保存先の作成に失敗した場合は一覧と選択を解除する。
    void RefreshWorkList();

    // 作業中のメタデータと作品一覧を照合し、現在の作品名を初回だけ同期する。先に一覧を取得しておく。
    void SynchronizeCurrentWorkIdentity();
    void SetWorkNameInput(const std::string& workName);

    // 入力中の作品名からファイル名を作り保存する。同じファイル名の作品があれば上書きする。
    bool SaveAsNamedWork();
    bool OverwriteCurrentWork();

    // ゴールと保存済みの作品名を確認し、上書き保存後に完成チェックを開始する。失敗理由はoutStatusMessageへ返す。
    void StartVerification(std::string& outStatusMessage);
    bool CompleteVerification(const std::string& workFileName);
    bool HasUnsavedChanges() const;

    // 未選択ならnullptr。一覧内の文字列への参照であり、一覧更新後には使わない。
    const std::string* FindSelectedWorkFileName() const;

    // 選択作品を作業ファイルへ反映し、現在の作品名を更新する。ステージの再読み込みは行わない。
    bool CopySelectedWorkToWorkingFile();

    // 選択作品を反映し、成功時にreloadSelectedWorkを呼ぶ。
    bool LoadSelectedWork();

    // 作業ファイルをテンプレートで置き換え、作品名を初期化する。成功時にreloadSelectedWorkを呼ぶ。
    bool CreateNewWorkingStage();
    bool DuplicateSelectedWork();

    // 選択作品を削除して一覧を更新する。現在の作品を削除した場合は保存先の識別情報も解除する。
    bool DeleteSelectedWork();
    bool IsSelectedWorkVerified() const;
    bool IsWorkVerified(const std::string& fileName) const;
    std::string ResolveDisplayName(const std::string& fileName) const;

private:
    void SetCurrentWork(const std::string& fileName, const std::string& displayName);
    bool SaveWorkToFile(const std::string& displayName, const std::string& fileName);
    bool SaveCurrentWorkForVerification(std::string& outWorkFileName, std::string& outErrorMessage);

    DebugEditorContext& mContext;
    UGCWorkState& mState;
    UGCWorkFileService mFileService;
    std::function<void()> mReloadSelectedWork;
};
