#pragma once

#include <optional>
#include <string>

class UGCSessionState {
public:
    void EnterEditor();
    void ToggleDebugPanel();
    bool StartPlaytest();
    bool StartVerification(const std::string& workFileName);
    bool ReturnToEditor();
    void Exit();

    void OpenWorkBrowser();
    void CloseWorkBrowser();
    void MarkClearCompletionPending();
    std::optional<std::string> ConsumeClearCompletion();

    bool IsModeActive() const;
    bool IsPlaytestActive() const;
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
    std::string mVerificationWorkFileName;
};
