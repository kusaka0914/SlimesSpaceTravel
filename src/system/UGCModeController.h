#pragma once

#include "gfx/debug/ugc/UGCClearTransitionState.h"
#include "gfx/debug/ugc/UGCSessionState.h"

#include <string>

class UGCModeRuntime;

// UGCの編集・試遊・クリア検証の遷移を管理し、実行処理をUGCModeRuntimeへ委譲する。
class UGCModeController {
public:
    explicit UGCModeController(UGCModeRuntime& runtime);

    bool StartMode();

    // 編集チュートリアルが未確認なら開始し、確認済みなら通常の編集モードを開始する。
    bool StartModeFromTitleSelection();

    // チュートリアル用Stageをテンプレートから作り直して開始する。コピー・開始失敗時はfalse。
    bool StartEditorTutorial();

    // 完了またはスキップを記録して通常編集へ移る。戻り値は通常編集の開始結果。
    bool FinishEditorTutorial(bool wasCompleted);
    void OpenWorkBrowser();
    void CloseWorkBrowser();

    // 編集モードから試遊へ移り、Stageを再読込する。状態上開始できない場合は何もしない。
    void StartPlaytest();
    void StartSavedWorkPlaytest();
    void StartClearVerification(const std::string& workFileName);

    // Stageを再読込して編集へ戻る。状態上戻れない場合は何もしない。
    void ReturnToEditor();
    void ExitMode();
    bool HandleGoalObtained();

    // Actor更新中のStage破棄を避けるため、クリア後の遷移を予約する。
    void HandleGoalCollectionFinished();

    // Actor更新の外側で呼び、予約されたクリア後の処理をフェード経由で実行する。
    void ProcessPendingClearCompletion();
    void MoveClearResultSelection(int delta);
    void ExecuteClearResultSelection();
    void ToggleDebugPanel();

    bool IsModeActive() const;
    bool IsPlaytestActive() const;
    bool IsVerificationActive() const;
    bool IsClearResultShowing() const;
    int GetClearResultSelection() const;
    bool IsDebugPanelShowing() const;
    bool IsWorkBrowserShowing() const;
    bool IsOrthographicView() const;
    void SetOrthographicView(bool isOrthographicView);
    bool IsEditorTutorialActive() const;

private:
    void StartPlaytest(UGCPlaytestPurpose purpose);
    bool StartModeWithStage(const std::string& yamlPath, bool isTutorial);
    bool HasSeenEditorTutorial() const;
    void CompleteModeExit(bool shouldOpenWorkBrowser);

    UGCModeRuntime& mRuntime;
    UGCSessionState mSessionState;
    UGCClearTransitionState mClearTransitionState;
    bool mIsEditorTutorialActive = false;
};
