#pragma once

class Game;
class UIRenderer;

class StateUIRenderer {
public:
    StateUIRenderer(Game* game, UIRenderer* renderer);

    void DrawStateUI();

private:
    void DrawBattleTutorial();
    void DrawBreakTutorial();
    void DrawJewelTutorial();
    void DrawJustDodgeTutorial();
    void DrawTalkWithNPC();
    void DrawStageClear();
    float CalculateAlpha() const;
    void DrawFadeInBg(float alpha);
    void DrawLoading();

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
