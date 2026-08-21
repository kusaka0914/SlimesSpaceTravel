#pragma once

#include <string>

class Game;
class UIRenderer;

class SceneUIRenderer {
public:
    SceneUIRenderer(Game* game, UIRenderer* renderer);

    void DrawTitle();
    void DrawOpening();
    void DrawEnding();
    void DrawCredits();
    void DrawGameOver();

private:
    void DrawStorybookPage(
        const std::string& trackId,
        const std::string& fallbackScene,
        int pageOffset = 0);
    void DrawOpeningIntro();
    void DrawOpeningTalkWithMother();
    void DrawOpeningTalkWithDoctor();

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
