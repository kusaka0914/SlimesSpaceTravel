#include "gfx/ui/HudRenderer.h"

// GLFWがOpenGLヘッダを読み込む前にGLEWを読み込む。
#include <GL/glew.h>

#include "actor/Planet.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "gfx/UIRenderer.h"
#include "system/SceneSystem.h"
#include <algorithm>
#include <string>
#include <vector>

HudRenderer::HudRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void HudRenderer::UpdateConsumptionFeedback(float deltaTime)
{
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined();
    if (mWasTwoPlayer != isTwoPlayer) {
        ResetConsumptionFeedback();
        mWasTwoPlayer = isTwoPlayer;
    }
    const auto* scene = mGame->GetSceneSystem();
    const bool allowsFeedback = scene && scene->IsPlaying() && !scene->IsTransitionActive() &&
                                !mGame->GetIsPauseMenuOpen() && !mGame->GetIsDebugEditorShowing();
    const auto& players = mGame->GetPlayers();
    for (std::size_t index = 0; index < mHpFeedback.size(); ++index) {
        const Player* player = isTwoPlayer ? (index < players.size() ? players[index] : nullptr)
                                           : (index == 0 ? mGame->GetMainPlayer() : nullptr);
        if (!player) {
            mHpFeedback[index].Reset();
            mJewelFeedback[index].Reset();
            continue;
        }
        const bool allowsPlayerFeedback = allowsFeedback && player->GetIsActive();
        mHpFeedback[index].Update(player->GetHp(), static_cast<int>(player->GetMaxHp()), deltaTime,
                                  allowsPlayerFeedback);
        mJewelFeedback[index].Update(static_cast<float>(player->GetJewelCount()), player->GetMaxJewelCount(), deltaTime,
                                     allowsPlayerFeedback);
    }
    mGuardFeedback.Update(static_cast<float>(mGame->GetPlayerSplitGuardCount()),
                          mGame->GetMaximumPlayerSplitGuardCount(), deltaTime, allowsFeedback && !isTwoPlayer);
}

void HudRenderer::ResetConsumptionFeedback()
{
    for (auto& feedback : mHpFeedback) {
        feedback.Reset();
    }
    for (auto& feedback : mJewelFeedback) {
        feedback.Reset();
    }
    mGuardFeedback.Reset();
}

void HudRenderer::DrawDamageFeedback()
{
    const auto* scene = mGame->GetSceneSystem();
    if (!scene || !scene->IsPlaying() || scene->IsTransitionActive() || mGame->GetIsPauseMenuOpen()) {
        return;
    }
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined();
    const float viewportHeight = mRenderer->GetFbHeight() * (isTwoPlayer ? 0.5f : 1.0f);
    const std::size_t count = isTwoPlayer ? 2 : 1;
    for (std::size_t index = 0; index < count; ++index) {
        mRenderer->DrawDamageVignette(viewportHeight * index, viewportHeight, 0.2f * mHpFeedback[index].GetStrength());
    }
}

void HudRenderer::DrawDefaultUI()
{
    DrawPlayerFatiguePromptUI();

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (players.empty()) {
        return;
    }

    const Player* mainPlayer = mGame->GetMainPlayer();

    if (mGame->IsInBase()) {
        return;
    }

    if (!mainPlayer || !mainPlayer->GetCurrentPlanet()) {
        return;
    }

    const int remainBoatPartsCount = mainPlayer->GetCurrentPlanet()->GetRemainBoatPartsCount();
    if (remainBoatPartsCount != 0) {
        DrawRemainPartsUI(remainBoatPartsCount);
    }

    DrawPlayerStatusUI();
}

void HudRenderer::DrawUGCPlaytestUI()
{
    DrawPlayerFatiguePromptUI();
    DrawPlayerStatusUI();

    if (mGame->GetIsUGCClearVerificationActive()) {
        DrawUGCClearVerificationGuide();
    }
}

void HudRenderer::DrawPlayerStatusUI()
{
    const std::vector<Player*>& players = mGame->GetPlayers();
    const Player* mainPlayer = mGame->GetMainPlayer();
    if (players.empty() || !mainPlayer) {
        return;
    }

    const bool isTwoPlayer = mGame->GetIsPlayer2Joined() && players.size() >= 2;
    if (!isTwoPlayer) {
        DrawPlayerStatusUIForPlayer(mainPlayer, 0.0f, 1.0f);
    } else {
        const float halfHeight = static_cast<float>(mRenderer->GetFbHeight()) * 0.5f;
        DrawPlayerStatusUIForPlayer(players[0], 0.0f, 0.5f);
        DrawPlayerStatusUIForPlayer(players[1], halfHeight, 0.5f);
    }
}

void HudRenderer::DrawPlayerFatiguePromptUI()
{
    const std::vector<Player*>& players = mGame->GetPlayers();
    if (players.empty()) {
        return;
    }

    const bool isTwoPlayer = mGame->GetIsPlayer2Joined() && players.size() >= 2;
    if (!isTwoPlayer) {
        DrawPlayerPromptUI(mGame->GetMainPlayer(), 0.0f, static_cast<float>(mRenderer->GetFbHeight()));
        return;
    }

    const float halfHeight = static_cast<float>(mRenderer->GetFbHeight()) * 0.5f;
    DrawPlayerPromptUI(players[0], 0.0f, halfHeight);
    DrawPlayerPromptUI(players[1], halfHeight, halfHeight);
}

void HudRenderer::DrawPlayerStatusUIForPlayer(const Player* player, float screenTopY, float uiScale)
{
    if (!player) {
        return;
    }

    const std::size_t index = mGame->GetIsPlayer2Joined() && player->GetPlayerNum() == 2 ? 1 : 0;
    DrawHpUI(static_cast<int>(player->GetHp()), static_cast<int>(player->GetMaxHp()), screenTopY, uiScale,
             mHpFeedback[index]);
    DrawJewelUI(player->GetJewelCount(), player->GetMaxJewelCount(), screenTopY, uiScale, mJewelFeedback[index]);

    if (!mGame->GetIsPlayer2Joined()) {
        DrawSplitGuardUI(screenTopY, uiScale);
    }
}

void HudRenderer::DrawPlayerPromptUI(const Player* player, float screenTopY, float screenHeight)
{
    if (!player) {
        return;
    }

    if (player->GetIsTired()) {
        DrawRecommendReduceTiredUI(player, screenTopY, screenHeight);
    }
}

void HudRenderer::DrawHpUI(int hp, int maximumHp, float screenTopY, float uiScale,
                           const ResourceConsumptionFeedback& feedback)
{
    const UIRenderer::TextureSlotFlash flash{
        feedback.GetFirstSlot(), feedback.GetEndSlot(), feedback.GetStrength(), {1.0f, 0.85f, 0.85f}};
    mRenderer->DrawLinedUpTextureSlots("default", "hpTexture", "hp", hp, maximumHp, 0.3f, screenTopY, uiScale, &flash);
}

void HudRenderer::DrawJewelUI(int jewelCount, int maximumJewelCount, float screenTopY, float uiScale,
                              const ResourceConsumptionFeedback& feedback)
{
    const UIRenderer::TextureSlotFlash flash{
        feedback.GetFirstSlot(), feedback.GetEndSlot(), feedback.GetStrength(), {0.6f, 0.85f, 1.0f}};
    mRenderer->DrawLinedUpTextureSlots("default", "jewelTexture", "jewel", jewelCount, maximumJewelCount, 0.3f,
                                       screenTopY, uiScale, &flash);
}

void HudRenderer::DrawSplitGuardUI(float screenTopY, float uiScale)
{
    const int maximumGuardCount = mGame->GetMaximumPlayerSplitGuardCount();
    if (maximumGuardCount <= 0) {
        return;
    }

    const int guardCount = mGame->GetPlayerSplitGuardCount();
    const UIRenderer::TextureSlotFlash flash{
        mGuardFeedback.GetFirstSlot(), mGuardFeedback.GetEndSlot(), mGuardFeedback.GetStrength(), {0.3f, 0.65f, 1.0f}};
    constexpr float depletedGuardBrightness = 0.3f;
    mRenderer->DrawLinedUpTextureSlots("default", "splitGuardTexture", "guard", guardCount, maximumGuardCount,
                                       depletedGuardBrightness, screenTopY, uiScale, &flash);
}

void HudRenderer::UpdateTalkableUIVisibility(const std::vector<Player*>& players, bool allowsPrompt)
{
    constexpr const char* screen = "default";
    constexpr const char* talkableTextId = "talkableText";
    constexpr const char* controllerTextureId = "talkableTextureForGameController";
    constexpr const char* keyboardTextureId = "talkableTextureForKeyboard";

    const SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool hasModalConversation = sceneSystem && (sceneSystem->IsTalkWithNPC() || sceneSystem->HasActiveTutorial());
    const Player* promptPlayer = nullptr;
    if (allowsPrompt && !hasModalConversation) {
        for (const Player* player : players) {
            if (sceneSystem && sceneSystem->CanStartTalkWithNPC(player)) {
                promptPlayer = player;
                break;
            }
        }
    }

    const bool shouldShowPrompt = promptPlayer != nullptr;
    const bool usesController = shouldShowPrompt && mRenderer->UsesControllerUI(promptPlayer);

    mRenderer->SetCustomUIElementVisible(screen, talkableTextId, shouldShowPrompt);
    mRenderer->SetCustomUIElementVisible(screen, controllerTextureId, shouldShowPrompt && usesController);
    mRenderer->SetCustomUIElementVisible(screen, keyboardTextureId, shouldShowPrompt && !usesController);
}

void HudRenderer::DrawRemainPartsUI(int remainBoatPartsCount)
{
    const auto remainPartsTextInfo = mRenderer->GetUILoadSystem()->GetTextInfo("default", "remainPartsText");
    if (!remainPartsTextInfo || remainPartsTextInfo->texts.empty()) {
        return;
    }

    const std::string remainText = remainPartsTextInfo->texts[0] + std::to_string(remainBoatPartsCount);
    mRenderer->DrawTextForElement(
        "default", "remainPartsText", mRenderer->GetFbWidth() - mRenderer->GetFbWidth() * remainPartsTextInfo->xRatio,
        mRenderer->GetFbWidth() * remainPartsTextInfo->yRatio,
        mRenderer->GetFbWidth() * remainPartsTextInfo->scaleRatio, remainText, remainPartsTextInfo->centerBased,
        {255, 255, 255, 255}, remainPartsTextInfo->rotationDegrees);
}

void HudRenderer::DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float screenHeight)
{
    mRenderer->DrawTextDependsOnPlayerInput(player, "state", "recommendReduceTiredText", screenTopY, screenHeight);
}

void HudRenderer::DrawUGCClearVerificationGuide()
{
    mRenderer->DrawSceneText("state", "ugcClearVerificationGuideText", 0, {255, 255, 255, 255});
}
