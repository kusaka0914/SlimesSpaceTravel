#pragma once

#include <vector>

class Game;
class Player;
class UIRenderer;

class HudRenderer {
public:
    HudRenderer(Game* game, UIRenderer* renderer);

    void DrawDefaultUI();
    void UpdateTalkableUIVisibility(
        const std::vector<Player*>& players,
        bool allowsPrompt);

private:
    void DrawPlayerStatusUI(const Player* player, float screenTopY, float uiScale);
    void DrawPlayerPromptUI(const Player* player, float screenTopY, float uiScale);
    void DrawHpUI(int hp, float screenTopY, float uiScale);
    void DrawJewelUI(int jewelCount, float screenTopY, float uiScale);
    void DrawDangerBg(int hp);
    void DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float uiScale);
    void DrawRemainPartsUI(int remainBoatPartsCount);

private:
    Game* mGame;
    UIRenderer* mRenderer;
};
