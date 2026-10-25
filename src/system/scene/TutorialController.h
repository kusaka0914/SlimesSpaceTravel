#pragma once

#include "system/tutorial/TutorialLibrary.h"

#include <cstdint>
#include <string>
#include <unordered_set>

class Game;
class GameProgressState;
class Player;
class Platform;
class UIState;

// チュートリアルの会話・操作目標を進め、再表示条件と完了記録を管理する。
class TutorialController {
public:
    TutorialController(Game* game, GameProgressState* gameProgressState, UIState* uiState);

    void Update(float deltaTime);

    // 既に再生中・定義なし・再表示不可ならfalse。Player未指定時は操作対象、次に先頭Playerを選ぶ。
    bool TryStart(const std::string& tutorialId, Player* tutorialPlayer = nullptr,
                  bool shouldIgnoreRepeatPolicy = false);
    bool Preview(const std::string& tutorialId);

    // 0始まりの指定ページからプレビューする。再表示制限を無視し、完了進行は記録しない。
    bool PreviewAtPage(const std::string& tutorialId, std::size_t pageIndex);

    // 再生とActor参照を解除する。shouldReturnToPlayingがtrueなら通常プレイへ戻る。
    void Stop(bool shouldReturnToPlaying = true);

    // 再生を止め、セッション内の表示・完了履歴をクリアする。保存進行は変更しない。
    void ResetForNewSession();
    bool ResumeAfterFocus();
    void TryAdvanceFromConfirm();

    void TryStartBattleTutorial();
    void TryStartJustDodgeTutorial();
    void OnEnemyLaunched();
    void OnStrongAttacked();
    void OnLanded();
    void OnPlayerSwitchSucceeded();
    void OnPlayerSplitMergeSucceeded();

    bool HasActiveTutorial() const;
    bool IsShowingConversation() const;
    bool IsShowingActionObjective() const;
    bool HasCompletedTutorial(const std::string& tutorialId) const;
    bool IsWaitingForPlayerAction() const;
    bool IsWaitingForPlayerSwitch() const;
    bool IsWaitingForPlayerJump() const;
    bool IsWaitingForPlayerSplitMerge() const;

    const TutorialDefinition* GetActiveDefinition() const;
    const TutorialPage* GetCurrentPage() const;
    Player* GetTutorialPlayer() const { return mTutorialPlayer; }
    const std::string& GetActiveTutorialId() const { return mActiveTutorialId; }
    std::uint64_t GetTutorialSessionSequence() const { return mTutorialSessionSequence; }

    TutorialLibrary& GetLibrary() { return mLibrary; }
    const TutorialLibrary& GetLibrary() const { return mLibrary; }

private:
    void BeginActionObjective();
    void AdvancePage();
    void FinishActiveTutorial();
    void CaptureCurrentPageActionBaseline();
    bool TryAdvanceFromCompletedAction();
    void AdvanceAfterCompletedAction();
    const Platform* FindObjectivePressureSwitch() const;
    bool IsTutorialPlayerOnPressureSwitchSide() const;
    bool IsTutorialPlayerNearPressureSwitch() const;
    bool IsTutorialPlayerPressingPressureSwitch() const;
    TutorialAdvanceCondition GetCurrentAdvanceCondition() const;

    Game* mGame = nullptr;
    GameProgressState* mGameProgressState = nullptr;
    UIState* mUIState = nullptr;

    TutorialLibrary mLibrary;
    std::unordered_set<std::string> mShownOnceTutorialIds;
    std::unordered_set<std::string> mCompletedTutorialIds;
    std::string mActiveTutorialId;
    Player* mTutorialPlayer = nullptr;
    Player* mActionPlayerAtPageStart = nullptr;
    std::uint64_t mJumpSequenceAtPageStart = 0;
    std::uint64_t mTutorialSessionSequence = 0;
    bool mHasJumpStartedOnCurrentPage = false;
    bool mIsActionObjectiveActive = false;
    bool mShouldRecordActiveTutorialCompletion = false;
};
