#pragma once

class Game;
class UIRenderer;

class SceneUIRenderer {
public:
    SceneUIRenderer(Game* game, UIRenderer* renderer);

    void DrawTitle();
    void DrawOpening();
    void DrawGameOver();

private:
    void DrawOpeningIntro();
    void DrawOpeningTalkWithMother();
    void DrawOpeningTalkWithDoctor();
    void DrawSkipUI();

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
