#pragma once

#include <optional>
#include <string>

enum class UGCPlaytestPurpose {
    EditorPreview,
    ClearVerification,
    SavedWork,
};

// UGCの編集・プレイテスト・完成チェック・クリア結果の状態遷移を管理する。
class UGCSessionState {
public:
    // 保留中の完成チェックと結果表示を解除し、正投影の編集状態へ初期化する。
    void EnterEditor();
    void ToggleDebugPanel();

    // UGCモード中のみ開始できる。開始時はデバッグ・結果表示を閉じ、透視投影へ切り替える。
    bool StartPlaytest(UGCPlaytestPurpose purpose);

    // 完成チェック対象を記録する。モード外または空の作品名ならfalse。プレイテストの開始は別途必要。
    bool StartVerification(const std::string& workFileName);

    // 完成チェックと結果表示を解除して編集へ戻る。UGCモード外ではfalse。
    bool ReturnToEditor();
    void Exit();

    void OpenWorkBrowser();
    void CloseWorkBrowser();
    void MarkClearCompletionPending();

    // 保留通知を1回だけ消費し、完成チェックの対象名を返す。対象名も解除する。
    // 通知がなければnullopt、完成チェック以外のクリアでは空文字列を返す。
    std::optional<std::string> ConsumeClearCompletion();
    void ShowClearResult();
    void MoveClearResultSelection(int delta, int itemCount);

    bool IsModeActive() const;
    bool IsPlaytestActive() const;
    bool IsVerificationActive() const;
    bool IsClearResultShowing() const;
    int GetClearResultSelection() const;
    UGCPlaytestPurpose GetPlaytestPurpose() const;
    bool IsDebugPanelShowing() const;
    bool IsWorkBrowserShowing() const;
    bool IsOrthographicView() const;
    void SetOrthographicView(bool isOrthographicView);

private:
    bool mIsModeActive = false;
    bool mIsPlaytestActive = false;
    bool mIsDebugPanelShowing = false;
    bool mIsWorkBrowserShowing = false;
    bool mIsOrthographicView = false;
    bool mIsClearCompletionPending = false;
    bool mIsClearResultShowing = false;
    int mClearResultSelection = 0;
    UGCPlaytestPurpose mPlaytestPurpose = UGCPlaytestPurpose::EditorPreview;
    std::string mVerificationWorkFileName;
};
