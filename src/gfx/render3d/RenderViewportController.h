#pragma once

class Game;
class Renderer3D;

class RenderViewportController {
public:
    RenderViewportController(Game* game, const Renderer3D* renderer);

    void DrawGameScreen(float fbWidth, float fbHeight) const;

private:
    void DrawGameScreenForSinglePerson(float fbWidth, float fbHeight) const;
    void DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const;

private:
    Game* mGame;
    const Renderer3D* mRenderer;
};
