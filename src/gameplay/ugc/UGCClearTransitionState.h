#pragma once

#include <optional>
#include <string>

enum class UGCClearDestination {
    Editor,
    WorkBrowser,
    ResultMenu,
};

// クリア後の遷移先と未処理の完了通知を保持し、遷移の重複開始を防ぐ。
class UGCClearTransitionState {
public:
    bool IsTransitionInProgress() const;
    bool HasPendingCompletion() const;

    // 未処理の通知または進行中の遷移があれば、新しい通知を無視する。
    void QueueCompletion(const std::string& workFileName, UGCClearDestination destination);

    // 未処理の作品名を返す。通知がなければ空文字列。BeginTransitionの前に取得する。
    const std::string& GetCompletedWorkFileName() const;
    UGCClearDestination GetDestination() const;

    // 未処理の通知を消費して遷移中にする。通知がなければ何もしない。
    void BeginTransition();
    void CompleteTransition();

private:
    bool mIsTransitionInProgress = false;
    std::optional<std::string> mPendingWorkFileName;
    UGCClearDestination mDestination = UGCClearDestination::Editor;
};
