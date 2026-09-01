#include "system/scene/TutorialController.h"

#include "Game.h"
#include "actor/Player.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "component/PlatformPressureSwitchComponent.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"

#include <glm/glm.hpp>

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
        !mIsActionObjectiveActive ||
        mGameProgressState->GetSceneState() !=
            GameProgressState::SceneState::Playing) {
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
    if (HasActiveTutorial() || !definition ||
        definition->GetPagesForControlStyle(
            mGame && mGame->IsAssistControlStyle()).empty() ||
        !mGame || !mGameProgressState || !mUIState) {
        return false;
    }

    const bool wasAlreadyCompleted =
        mGame->HasCompletedTutorial(tutorialId);
    const bool wasAlreadyShownThisSession =
        mShownOnceTutorialIds.contains(tutorialId);
    if (!ignoreRepeatPolicy &&
        (wasAlreadyCompleted || wasAlreadyShownThisSession) &&
        definition->repeatPolicy ==
            TutorialRepeatPolicy::OnceEver) {
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
            TutorialRepeatPolicy::OnceEver) {
        mShownOnceTutorialIds.insert(tutorialId);
    }
    mShouldRecordActiveTutorialCompletion =
        !ignoreRepeatPolicy;
    mIsActionObjectiveActive = false;

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
    mIsActionObjectiveActive = false;
    mShouldRecordActiveTutorialCompletion = false;
    if (mUIState) {
        mUIState->FinishTutorial();
    }
    if (returnToPlaying && mGame) {
        mGame->StartPlayingScene();
    }
}

bool TutorialController::ResumeAfterFocus()
{
    if (!HasActiveTutorial() || !mGameProgressState) {
        return false;
    }

    const GameProgressState::SceneState resumedState =
        mIsActionObjectiveActive
            ? GameProgressState::SceneState::Playing
            : GameProgressState::SceneState::Talking;
    mGameProgressState->SetCurrentSceneState(resumedState);

    if (!mIsActionObjectiveActive &&
        mGame && mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("message_se");
    }
    return true;
}

void TutorialController::TryAdvanceFromConfirm()
{
    if (!HasActiveTutorial() || mIsActionObjectiveActive) {
        return;
    }

    if (GetCurrentAdvanceCondition() ==
        TutorialAdvanceCondition::Confirm) {
        AdvancePage();
        return;
    }

    BeginActionObjective();
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

    if (GetCurrentAdvanceCondition() ==
        TutorialAdvanceCondition::PlayerSplitMerge) {
        AdvanceAfterCompletedAction();
        return;
    }
    TryAdvanceFromCompletedAction();
}

bool TutorialController::HasActiveTutorial() const
{
    return GetActiveDefinition() != nullptr;
}

bool TutorialController::IsShowingConversation() const
{
    return HasActiveTutorial() && !mIsActionObjectiveActive &&
           mGameProgressState &&
           mGameProgressState->GetSceneState() ==
               GameProgressState::SceneState::Talking;
}

bool TutorialController::IsShowingActionObjective() const
{
    return HasActiveTutorial() && mIsActionObjectiveActive &&
           mGameProgressState &&
           mGameProgressState->GetSceneState() ==
               GameProgressState::SceneState::Playing;
}

bool TutorialController::HasCompletedTutorial(
    const std::string& tutorialId) const
{
    return !tutorialId.empty() &&
           (mCompletedTutorialIds.contains(tutorialId) ||
            (mGame && mGame->HasCompletedTutorial(tutorialId)));
}

bool TutorialController::IsWaitingForPlayerAction() const
{
    return IsShowingActionObjective() &&
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
    if (!IsWaitingForPlayerAction()) {
        return false;
    }

    const TutorialAdvanceCondition condition =
        GetCurrentAdvanceCondition();
    return condition == TutorialAdvanceCondition::PlayerSplitMerge ||
           condition == TutorialAdvanceCondition::PlayerSplit ||
           condition == TutorialAdvanceCondition::PlayerMerge;
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

    mIsActionObjectiveActive = false;
    const TutorialPage* nextPage = GetCurrentPage();
    const bool startsDirectlyAsObjective =
        nextPage &&
        nextPage->advanceCondition !=
            TutorialAdvanceCondition::Confirm &&
        !nextPage->HasConversationText(
            mGame && mGame->IsGameControllerConnected());
    if (startsDirectlyAsObjective) {
        BeginActionObjective();
        return;
    }

    mGameProgressState->SetCurrentSceneState(
        GameProgressState::SceneState::Talking);
    if (mGame && mGame->GetAudioSystem()) {
        mGame->GetAudioSystem()->PlaySE("message_se");
    }
}

void TutorialController::FinishActiveTutorial()
{
    if (!mActiveTutorialId.empty()) {
        mCompletedTutorialIds.insert(mActiveTutorialId);
        if (mShouldRecordActiveTutorialCompletion && mGame) {
            mGame->MarkTutorialCompleted(mActiveTutorialId);
        }
    }
    mActiveTutorialId.clear();
    mTutorialPlayer = nullptr;
    mActionPlayerAtPageStart = nullptr;
    mIsActionObjectiveActive = false;
    mShouldRecordActiveTutorialCompletion = false;
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

void TutorialController::BeginActionObjective()
{
    if (!mGameProgressState ||
        GetCurrentAdvanceCondition() ==
            TutorialAdvanceCondition::Confirm) {
        return;
    }

    CaptureCurrentPageActionBaseline();
    mIsActionObjectiveActive = true;
    mGameProgressState->SetCurrentSceneState(
        GameProgressState::SceneState::Playing);
    TryAdvanceFromCompletedAction();
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

        AdvanceAfterCompletedAction();
        return true;
    }

    case TutorialAdvanceCondition::PlayerSplitMerge:
        return false;

    case TutorialAdvanceCondition::PlayerSplit:
        if (!mGame || !mGame->GetIsPlayerSplit()) {
            return false;
        }
        AdvanceAfterCompletedAction();
        return true;

    case TutorialAdvanceCondition::PlayerMerge:
        if (!mGame || mGame->GetIsPlayerSplit()) {
            return false;
        }
        AdvanceAfterCompletedAction();
        return true;

    case TutorialAdvanceCondition::ApproachPressureSwitch:
        if (!IsTutorialPlayerNearPressureSwitch()) {
            return false;
        }
        AdvanceAfterCompletedAction();
        return true;

    case TutorialAdvanceCondition::PressPressureSwitch:
        if (!IsTutorialPlayerPressingPressureSwitch()) {
            return false;
        }
        AdvanceAfterCompletedAction();
        return true;

    case TutorialAdvanceCondition::Confirm:
    default:
        return false;
    }
}

void TutorialController::AdvanceAfterCompletedAction()
{
    if (mGame && mGame->GetControlledPlayer()) {
        mTutorialPlayer = mGame->GetControlledPlayer();
    }
    AdvancePage();
}

const Platform* TutorialController::FindObjectivePressureSwitch() const
{
    const TutorialPage* page = GetCurrentPage();
    if (!page || page->objectivePlatformId.empty()) {
        return nullptr;
    }

    const Player* player =
        mGame ? mGame->GetControlledPlayer() : nullptr;
    if (!player) {
        player = mTutorialPlayer;
    }
    const Planet* planet = player ? player->GetCurrentPlanet() : nullptr;
    if (!planet) {
        return nullptr;
    }

    for (const Platform* platform : planet->GetPlatforms()) {
        if (platform &&
            platform->GetPlatformId() == page->objectivePlatformId &&
            platform->GetPressureSwitchComponent()) {
            return platform;
        }
    }
    return nullptr;
}

bool TutorialController::IsTutorialPlayerNearPressureSwitch() const
{
    const Player* player =
        mGame ? mGame->GetControlledPlayer() : nullptr;
    if (!player) {
        player = mTutorialPlayer;
    }
    const Platform* pressureSwitch =
        FindObjectivePressureSwitch();
    if (!player || !pressureSwitch) {
        return false;
    }

    constexpr float discoveryDistanceWorldUnits = 3.0f;
    const glm::vec3 offset =
        pressureSwitch->GetPos() - player->GetPos();
    return glm::dot(offset, offset) <=
           discoveryDistanceWorldUnits *
               discoveryDistanceWorldUnits;
}

bool TutorialController::IsTutorialPlayerPressingPressureSwitch() const
{
    const Platform* platform = FindObjectivePressureSwitch();
    const PlatformPressureSwitchComponent* pressureSwitch =
        platform ? platform->GetPressureSwitchComponent() : nullptr;
    return pressureSwitch && pressureSwitch->GetIsPressed();
}

TutorialAdvanceCondition
TutorialController::GetCurrentAdvanceCondition() const
{
    const TutorialPage* page = GetCurrentPage();
    return page
               ? page->advanceCondition
               : TutorialAdvanceCondition::Confirm;
}
