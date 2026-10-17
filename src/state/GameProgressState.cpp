#include "state/GameProgressState.h"

GameProgressState::GameProgressState()
    : mIsFirstBreak(false),
      mIsFirstStrongAttack(false),
      mCurrentSceneState(SceneState::Title),
      mNextSceneState(SceneState::None)
{
}
