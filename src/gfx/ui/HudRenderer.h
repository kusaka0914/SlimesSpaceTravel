#pragma once

#include "gfx/ui/ResourceConsumptionFeedback.h"

#include <array>
#include <vector>

class Game;
class Player;
class UIRenderer;

// 通常プレイとUGCプレイテストのHUD・会話開始案内を描画する。GameとUIRendererは非所有参照。
class HudRenderer {
public:
    HudRenderer(Game* game, UIRenderer* renderer);

    // 通常プレイのHUDを描く。拠点では疲労案内のみ表示する。
    void DrawDefaultUI();

    // 残量変化を毎フレーム1回取得する。ステージ再読込時はResetConsumptionFeedbackで履歴を捨てる。
    void UpdateConsumptionFeedback(float deltaTime);
    void ResetConsumptionFeedback();
    void DrawDamageFeedback();

    // プレイテスト用HUDと、クリア検証中の案内を描く。
    void DrawUGCPlaytestUI();

    // 会話開始案内の表示を更新する。allowsPromptがfalse、会話中、チュートリアル中なら非表示。
    void UpdateTalkableUIVisibility(const std::vector<Player*>& players, bool allowsPrompt);

private:
    void DrawPlayerStatusUI();
    void DrawPlayerFatiguePromptUI();
    void DrawPlayerStatusUIForPlayer(const Player* player, float screenTopY, float uiScale);
    void DrawPlayerPromptUI(const Player* player, float screenTopY, float screenHeight);
    void DrawHpUI(int hp, int maximumHp, float screenTopY, float uiScale, const ResourceConsumptionFeedback& feedback);
    void DrawJewelUI(int jewelCount, int maximumJewelCount, float screenTopY, float uiScale,
                     const ResourceConsumptionFeedback& feedback);
    void DrawSplitGuardUI(float screenTopY, float uiScale);
    void DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float screenHeight);
    void DrawRemainPartsUI(int remainBoatPartsCount);
    void DrawUGCClearVerificationGuide();

private:
    Game* mGame;
    UIRenderer* mRenderer;
    std::array<ResourceConsumptionFeedback, 2> mHpFeedback;
    std::array<ResourceConsumptionFeedback, 2> mJewelFeedback;
    ResourceConsumptionFeedback mGuardFeedback;
    bool mWasTwoPlayer = false;
};
