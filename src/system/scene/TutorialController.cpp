#include "system/scene/TutorialController.h"

#include "Game.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"

TutorialController::TutorialController(Game* game, GameProgressState* gameProgressState, UIState* uiState)
    : mGame(game),
      mGameProgressState(gameProgressState),
      mUIState(uiState)
{
}

void TutorialController::TryStartBattleTutorial()
{
    const Player* mainPlayer = mGame->GetMainPlayer();
    if (!mainPlayer || mainPlayer->GetCurrentPlanetNum() != 1) {
        return;
    }

    if (mUIState->GetIsBattleTutorialShown()) {
        return;
    }

    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Battle);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mUIState->SetIsBattleTutorialShown(true);
}

void TutorialController::TryStartJustDodgeTutorial()
{
    const Player* mainPlayer = mGame->GetMainPlayer();
    if (!mainPlayer || mainPlayer->GetCurrentPlanetNum() != 2) {
        return;
    }

    if (mUIState->GetIsJustDodgeTutorialShown()) {
        return;
    }

    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::JustDodge);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mUIState->SetIsJustDodgeTutorialShown(true);
}

void TutorialController::OnEnemyLaunched()
{
    if (mGameProgressState->GetIsFirstBreak()) {
        return;
    }

    mGameProgressState->SetIsFirstBreak(true);
    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Break);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
}

void TutorialController::OnStrongAttacked()
{
    if (!mGameProgressState->GetIsFirstBreak() || mGameProgressState->GetIsFirstStrongAttack()) {
        return;
    }

    mGameProgressState->SetIsFirstStrongAttack(true);
}

void TutorialController::OnLanded()
{
    if (!mGameProgressState->GetIsFirstStrongAttack() || mUIState->GetIsSpecialAttackTutorialShown()) {
        return;
    }

    mUIState->SetIsSpecialAttackTutorialShown(true);
    mUIState->SetCurrentTutorialKind(UIState::TutorialKind::Jewel);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
}
