#pragma once

class Game;
class Player;
class UIRenderer;

class HudRenderer {
public:
    HudRenderer(Game* game, UIRenderer* renderer);

    void DrawDefaultUI();

private:
    void DrawOperationSupportUI();
    void DrawPlayerStatusUI(const Player* player, float screenTopY, float uiScale);
    void DrawPlayerPromptUI(const Player* player, float screenTopY, float uiScale);
    void DrawHpUI(int hp, float screenTopY, float uiScale);
    void DrawJewelUI(int jewelCount, float screenTopY, float uiScale);
    void DrawDangerBg(int hp);
    void DrawTalkableUI(const Player* player, float screenTopY, float uiScale);
    void DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float uiScale);
    void DrawRemainPartsUI(int remainBoatPartsCount);

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
