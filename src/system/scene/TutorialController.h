#pragma once

#include "system/tutorial/TutorialLibrary.h"

#include <cstdint>
#include <string>
#include <unordered_set>

class Game;
class GameProgressState;
class Player;
class UIState;

class TutorialController {
public:
    TutorialController(
        Game* game,
        GameProgressState* gameProgressState,
        UIState* uiState);

    void Update(float deltaTime);

    bool TryStart(
        const std::string& tutorialId,
        Player* tutorialPlayer = nullptr,
        bool ignoreRepeatPolicy = false);
    bool Preview(const std::string& tutorialId);
    bool PreviewAtPage(
        const std::string& tutorialId,
        std::size_t pageIndex);
    void Stop(bool returnToPlaying = true);
    void TryAdvanceFromConfirm();

    void TryStartBattleTutorial();
    void TryStartJustDodgeTutorial();
    void OnEnemyLaunched();
    void OnStrongAttacked();
    void OnLanded();
    void OnPlayerSwitchSucceeded();
    void OnPlayerSplitMergeSucceeded();

    bool HasActiveTutorial() const;
    bool HasCompletedTutorial(const std::string& tutorialId) const;
    bool IsWaitingForPlayerAction() const;
    bool IsWaitingForPlayerSwitch() const;
    bool IsWaitingForPlayerJump() const;
    bool IsWaitingForPlayerSplitMerge() const;

    const TutorialDefinition* GetActiveDefinition() const;
    const TutorialPage* GetCurrentPage() const;
    Player* GetTutorialPlayer() const { return mTutorialPlayer; }
    const std::string& GetActiveTutorialId() const
    {
        return mActiveTutorialId;
    }
    std::uint64_t GetTutorialSessionSequence() const
    {
        return mTutorialSessionSequence;
    }

    TutorialLibrary& GetLibrary() { return mLibrary; }
    const TutorialLibrary& GetLibrary() const { return mLibrary; }

private:
    void AdvancePage();
    void FinishActiveTutorial();
    void CaptureCurrentPageActionBaseline();
    bool TryAdvanceFromCompletedAction();
    TutorialAdvanceCondition GetCurrentAdvanceCondition() const;

private:
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
    bool mShouldRecordActiveTutorialCompletion = false;
};
