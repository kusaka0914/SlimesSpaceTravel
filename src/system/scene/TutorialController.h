#pragma once

class Game;
class GameProgressState;
class UIState;

class TutorialController {
public:
    TutorialController(Game* game, GameProgressState* gameProgressState, UIState* uiState);

    void TryStartBattleTutorial();
    void TryStartJustDodgeTutorial();

    void OnEnemyLaunched();
    void OnStrongAttacked();
    void OnLanded();

private:
    Game* mGame;
    GameProgressState* mGameProgressState;
    UIState* mUIState;
};
