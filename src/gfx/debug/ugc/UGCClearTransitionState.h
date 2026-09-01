#pragma once

#include <optional>
#include <string>

enum class UGCClearDestination {
    Editor,
    WorkBrowser,
    ResultMenu,
};

class UGCClearTransitionState {
public:
    bool IsTransitionInProgress() const;
    bool HasPendingCompletion() const;
    void QueueCompletion(
        const std::string& workFileName,
        UGCClearDestination destination);
    const std::string& GetCompletedWorkFileName() const;
    UGCClearDestination GetDestination() const;
    void BeginTransition();
    void CompleteTransition();

private:
    bool mIsTransitionInProgress = false;
    std::optional<std::string> mPendingWorkFileName;
    UGCClearDestination mDestination = UGCClearDestination::Editor;
};
