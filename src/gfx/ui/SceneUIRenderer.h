#pragma once

#include <string>

class Game;
class UIRenderer;

// タイトル・会話背景・クレジットなど、SceneSystemに応じた画面を描画する。GameとUIRendererは非所有参照。
class SceneUIRenderer {
public:
    SceneUIRenderer(Game* game, UIRenderer* renderer);

    void DrawTitle();
    void DrawOpening();
    void DrawEnding();
    void DrawCredits();
    void DrawGameOver();
    void DrawUGCClearResult();
    void DrawTgsThankYou();
    void DrawTgsRemainingTimeNotice();

private:
    void DrawStorybookPage(const std::string& trackId, const std::string& fallbackScene, int pageOffset = 0);
    void DrawOpeningIntro();
    void DrawOpeningTalkWithMother();
    void DrawOpeningTalkWithDoctor();

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
