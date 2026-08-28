#pragma once

#include <optional>
#include <string>

class UGCClearTransitionState {
public:
    bool IsTransitionInProgress() const;
    bool HasPendingCompletion() const;
    void QueueCompletion(const std::string& workFileName);
    const std::string& GetCompletedWorkFileName() const;
    void BeginTransition();
    void CompleteTransition();

private:
    bool mIsTransitionInProgress = false;
    std::optional<std::string> mPendingWorkFileName;
};
