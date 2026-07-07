#pragma once

class Game;
class UIRenderer;

class PauseMenuRenderer {
public:
    PauseMenuRenderer(Game* game, UIRenderer* renderer);

    void Draw();

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
