#pragma once

#include <optional>
#include <string>

enum class UGCPlaytestPurpose {
    EditorPreview,
    ClearVerification,
    SavedWork,
};

class UGCSessionState {
public:
    void EnterEditor();
    void ToggleDebugPanel();
    bool StartPlaytest(UGCPlaytestPurpose purpose);
    bool StartVerification(const std::string& workFileName);
    bool ReturnToEditor();
    void Exit();

    void OpenWorkBrowser();
    void CloseWorkBrowser();
    void MarkClearCompletionPending();
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
    UGCPlaytestPurpose mPlaytestPurpose =
        UGCPlaytestPurpose::EditorPreview;
    std::string mVerificationWorkFileName;
};
