#include "system/scene/TutorialController.h"

#include "Game.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"

TutorialController::TutorialController(
    Game* game,
    GameProgressState* gameProgressState,
    UIState* uiState)
    : mGame(game),
      mGameProgressState(gameProgressState),
      mUIState(uiState)
{
}

void TutorialController::Update(float)
{
    if (!HasActiveTutorial() || !mGameProgressState ||
        mGameProgressState->GetSceneState() !=
            GameProgressState::SceneState::Talking) {
        return;
    }

    TryAdvanceFromCompletedAction();
}

bool TutorialController::TryStart(
    const std::string& tutorialId,
    Player* tutorialPlayer,
    bool ignoreRepeatPolicy)
{
    const TutorialDefinition* definition =
        mLibrary.Find(tutorialId);
    if (!definition ||
        definition->GetPagesForControlStyle(
            mGame && mGame->IsAssistControlStyle()).empty() ||
        !mGame || !mGameProgressState || !mUIState) {
        return false;
    }

    const bool wasAlreadyShown =
        mShownOnceTutorialIds.contains(tutorialId);
    if (!ignoreRepeatPolicy && wasAlreadyShown &&
        definition->repeatPolicy ==
            TutorialRepeatPolicy::OncePerSession) {
        return false;
    }

    ++mTutorialSessionSequence;
    mActiveTutorialId = tutorialId;
    mTutorialPlayer =
        tutorialPlayer
            ? tutorialPlayer
            : mGame->GetControlledPlayer();
    if (!mTutorialPlayer) {
        mTutorialPlayer = mGame->GetMainPlayer();
    }

    if (!ignoreRepeatPolicy &&
        definition->repeatPolicy ==
            TutorialRepeatPolicy::OncePerSession) {
        mShownOnceTutorialIds.insert(tutorialId);
    }

    mUIState->SetCurrentTalkWith(UIState::TalkWith::None);
    mUIState->SetTalkUIIndex(0);
    mGameProgressState->SetCurrentSceneState(
        GameProgressState::SceneState::Talking);
    CaptureCurrentPageActionBaseline();

    if (mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("message_se");
    }
    return true;
}

bool TutorialController::Preview(
    const std::string& tutorialId)
{
    return PreviewAtPage(tutorialId, 0);
}

bool TutorialController::PreviewAtPage(
    const std::string& tutorialId,
    std::size_t pageIndex)
{
    const TutorialDefinition* definition =
        mLibrary.Find(tutorialId);
    if (!definition ||
        pageIndex >= definition->GetPagesForControlStyle(
                         mGame && mGame->IsAssistControlStyle()).size()) {
        return false;
    }

    Stop(false);
    if (!TryStart(
        tutorialId,
        mGame ? mGame->GetControlledPlayer() : nullptr,
        true)) {
        return false;
    }

    mUIState->SetTalkUIIndex(static_cast<int>(pageIndex));
    CaptureCurrentPageActionBaseline();
    return true;
}

void TutorialController::Stop(bool returnToPlaying)
{
    if (!HasActiveTutorial()) {
        return;
    }

    mActiveTutorialId.clear();
    mTutorialPlayer = nullptr;
    mActionPlayerAtPageStart = nullptr;
    if (mUIState) {
        mUIState->FinishTutorial();
    }
    if (returnToPlaying && mGame) {
        mGame->StartPlayingScene();
    }
}

void TutorialController::TryAdvanceFromConfirm()
{
    if (!HasActiveTutorial() ||
        GetCurrentAdvanceCondition() !=
            TutorialAdvanceCondition::Confirm) {
        return;
    }
    AdvancePage();
}

void TutorialController::TryStartBattleTutorial()
{
    const Player* mainPlayer =
        mGame ? mGame->GetMainPlayer() : nullptr;
    if (!mainPlayer ||
        mainPlayer->GetCurrentPlanetNum() != 1) {
        return;
    }

    TryStart("battle_basic");
}

void TutorialController::TryStartJustDodgeTutorial()
{
    const Player* mainPlayer =
        mGame ? mGame->GetMainPlayer() : nullptr;
    if (!mainPlayer ||
        mainPlayer->GetCurrentPlanetNum() != 2) {
        return;
    }

    TryStart("just_dodge");
}

void TutorialController::OnEnemyLaunched()
{
    if (!mGameProgressState ||
        mGameProgressState->GetIsFirstBreak()) {
        return;
    }

    mGameProgressState->SetIsFirstBreak(true);
    // Assist style teaches launching as part of Y/J mashing, so it does not
    // need the separate guard-break / aerial-attack tutorial.
    if (mGame && mGame->IsAssistControlStyle()) {
        return;
    }
    TryStart("enemy_break");
}

void TutorialController::OnStrongAttacked()
{
    if (!mGameProgressState ||
        !mGameProgressState->GetIsFirstBreak() ||
        mGameProgressState->GetIsFirstStrongAttack()) {
        return;
    }

    mGameProgressState->SetIsFirstStrongAttack(true);
}

void TutorialController::OnLanded()
{
    if (!mGameProgressState ||
        !mGameProgressState->GetIsFirstStrongAttack()) {
        return;
    }

    TryStart("jewel_usage");
}

void TutorialController::OnPlayerSwitchSucceeded()
{
    if (!IsWaitingForPlayerSwitch()) {
        return;
    }

    mTutorialPlayer = mGame ? mGame->GetControlledPlayer() : nullptr;
    AdvancePage();
}

void TutorialController::OnPlayerSplitMergeSucceeded()
{
    if (!IsWaitingForPlayerSplitMerge()) {
        return;
    }

    mTutorialPlayer = mGame ? mGame->GetControlledPlayer() : nullptr;
    AdvancePage();
}

bool TutorialController::HasActiveTutorial() const
{
    return GetActiveDefinition() != nullptr;
}

bool TutorialController::IsWaitingForPlayerAction() const
{
    return HasActiveTutorial() && mGameProgressState &&
           mGameProgressState->GetSceneState() ==
               GameProgressState::SceneState::Talking &&
           GetCurrentAdvanceCondition() !=
               TutorialAdvanceCondition::Confirm;
}

bool TutorialController::IsWaitingForPlayerSwitch() const
{
    return IsWaitingForPlayerAction() &&
           GetCurrentAdvanceCondition() ==
               TutorialAdvanceCondition::PlayerSwitch;
}

bool TutorialController::IsWaitingForPlayerJump() const
{
    return IsWaitingForPlayerAction() &&
           GetCurrentAdvanceCondition() ==
               TutorialAdvanceCondition::Jump;
}

bool TutorialController::IsWaitingForPlayerSplitMerge() const
{
    return IsWaitingForPlayerAction() &&
           GetCurrentAdvanceCondition() ==
               TutorialAdvanceCondition::PlayerSplitMerge;
}

const TutorialDefinition*
TutorialController::GetActiveDefinition() const
{
    return mActiveTutorialId.empty()
               ? nullptr
               : mLibrary.Find(mActiveTutorialId);
}

const TutorialPage* TutorialController::GetCurrentPage() const
{
    const TutorialDefinition* definition =
        GetActiveDefinition();
    if (!definition || !mUIState) {
        return nullptr;
    }

    const int pageIndex = mUIState->GetTalkUIIndex();
    const std::vector<TutorialPage>& pages =
        definition->GetPagesForControlStyle(
            mGame && mGame->IsAssistControlStyle());
    if (pageIndex < 0 ||
        pageIndex >= static_cast<int>(pages.size())) {
        return nullptr;
    }
    return &pages[
        static_cast<std::size_t>(pageIndex)];
}

void TutorialController::AdvancePage()
{
    if (!mUIState) {
        return;
    }

    mUIState->IncTalkUIIndex();
    const TutorialDefinition* definition =
        GetActiveDefinition();
    if (!definition ||
        mUIState->GetTalkUIIndex() >= static_cast<int>(
            definition->GetPagesForControlStyle(
                mGame && mGame->IsAssistControlStyle()).size())) {
        FinishActiveTutorial();
        return;
    }

    CaptureCurrentPageActionBaseline();
    if (mGame && mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("message_se");
    }
}

void TutorialController::FinishActiveTutorial()
{
    mActiveTutorialId.clear();
    mTutorialPlayer = nullptr;
    mActionPlayerAtPageStart = nullptr;
    if (mUIState) {
        mUIState->FinishTutorial();
    }
    if (mGame) {
        mGame->StartPlayingScene();
    }
}

void TutorialController::CaptureCurrentPageActionBaseline()
{
    mActionPlayerAtPageStart =
        mGame ? mGame->GetControlledPlayer() : nullptr;
    mJumpSequenceAtPageStart =
        mActionPlayerAtPageStart
            ? mActionPlayerAtPageStart->GetJumpSequence()
            : 0;
    mHasJumpStartedOnCurrentPage = false;
}

bool TutorialController::TryAdvanceFromCompletedAction()
{
    switch (GetCurrentAdvanceCondition()) {
    case TutorialAdvanceCondition::PlayerSwitch:
        return false;

    case TutorialAdvanceCondition::Jump: {
        Player* controlledPlayer =
            mGame ? mGame->GetControlledPlayer() : nullptr;
        if (!controlledPlayer ||
            controlledPlayer != mActionPlayerAtPageStart) {
            return false;
        }

        if (controlledPlayer->GetJumpSequence() >
            mJumpSequenceAtPageStart) {
            mHasJumpStartedOnCurrentPage = true;
        }
        if (!mHasJumpStartedOnCurrentPage ||
            !controlledPlayer->GetOnGround()) {
            return false;
        }

        AdvancePage();
        return true;
    }

    case TutorialAdvanceCondition::PlayerSplitMerge:
        return false;

    case TutorialAdvanceCondition::Confirm:
    default:
        return false;
    }
}

TutorialAdvanceCondition
TutorialController::GetCurrentAdvanceCondition() const
{
    const TutorialPage* page = GetCurrentPage();
    return page
               ? page->advanceCondition
               : TutorialAdvanceCondition::Confirm;
}
