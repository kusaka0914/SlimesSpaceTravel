#include "gfx/ui/HudRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include <string>
#include <vector>

HudRenderer::HudRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void HudRenderer::DrawDefaultUI()
{
    const std::vector<Player*>& players = mGame->GetPlayers();
    if (players.empty()) {
        return;
    }

    const bool isTwoPlayer = mGame->GetIsPlayer2Joined() && players.size() >= 2;
    const float halfHeight = static_cast<float>(mRenderer->GetFbHeight()) * 0.5f;
    const Player* mainPlayer = mGame->GetMainPlayer();

    if (!isTwoPlayer) {
        DrawPlayerPromptUI(mainPlayer, 0.0f, 1.0f);
    } else {
        DrawPlayerPromptUI(players[0], 0.0f, 0.5f);
        DrawPlayerPromptUI(players[1], halfHeight, 0.5f);
    }

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

    if (!isTwoPlayer) {
        DrawPlayerStatusUI(mainPlayer, 0.0f, 1.0f);
    } else {
        DrawPlayerStatusUI(players[0], 0.0f, 0.5f);
        DrawPlayerStatusUI(players[1], halfHeight, 0.5f);
    }
}

void HudRenderer::DrawPlayerStatusUI(const Player* player, float screenTopY, float uiScale)
{
    if (!player) {
        return;
    }

    const int hp = player->GetHp();
    if (hp > 0) {
        DrawHpUI(hp, screenTopY, uiScale);
    }

    const int jewelCount = player->GetJewelCount();
    if (jewelCount > 0) {
        DrawJewelUI(jewelCount, screenTopY, uiScale);
    }
}

void HudRenderer::DrawPlayerPromptUI(const Player* player, float screenTopY, float uiScale)
{
    if (!player) {
        return;
    }

    if (player->GetIsTired()) {
        DrawRecommendReduceTiredUI(player, screenTopY, uiScale);
    }
}

void HudRenderer::DrawHpUI(int hp, float screenTopY, float uiScale)
{
    const float hpGap = mRenderer->GetFbWidth() / 28.0f;
    mRenderer->DrawLinedUpTexture("default", "hpTexture", "hp", hpGap, hp, screenTopY, uiScale);
}

void HudRenderer::DrawDangerBg(int hp)
{
    if (hp == 3) {
        mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(),
                          {1.0f, 0.0f, 0.0f, 0.05f});
    } else if (hp == 2) {
        mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(),
                          {1.0f, 0.0f, 0.0f, 0.1f});
    } else if (hp == 1) {
        mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(),
                          {1.0f, 0.0f, 0.0f, 0.2f});
    }
}

void HudRenderer::DrawJewelUI(int jewelCount, float screenTopY, float uiScale)
{
    const float jewelGap = mRenderer->GetFbWidth() / 20.0f;
    mRenderer->DrawLinedUpTexture("default", "jewelTexture", "jewel", jewelGap, jewelCount, screenTopY, uiScale);
}

void HudRenderer::UpdateTalkableUIVisibility(
    const std::vector<Player*>& players,
    bool allowsPrompt)
{
    constexpr const char* screen = "default";
    constexpr const char* talkableTextId = "talkableText";
    constexpr const char* controllerTextureId =
        "talkableTextureForGameController";
    constexpr const char* keyboardTextureId =
        "talkableTextureForKeyboard";

    const Player* promptPlayer = nullptr;
    if (allowsPrompt) {
        for (const Player* player : players) {
            if (mGame->GetSceneSystem()->CanStartTalkWithNPC(player)) {
                promptPlayer = player;
                break;
            }
        }
    }

    const bool shouldShowPrompt = promptPlayer != nullptr;
    const bool usesController =
        shouldShowPrompt &&
        mRenderer->UsesControllerUI(promptPlayer);

    mRenderer->SetCustomUIElementVisible(
        screen,
        talkableTextId,
        shouldShowPrompt);
    mRenderer->SetCustomUIElementVisible(
        screen,
        controllerTextureId,
        shouldShowPrompt && usesController);
    mRenderer->SetCustomUIElementVisible(
        screen,
        keyboardTextureId,
        shouldShowPrompt && !usesController);
}

void HudRenderer::DrawRemainPartsUI(int remainBoatPartsCount)
{
    const auto remainPartsTextInfo = mRenderer->GetUILoadSystem()->GetTextInfo("default", "remainPartsText");
    if (!remainPartsTextInfo || remainPartsTextInfo->texts.empty()) {
        return;
    }

    const std::string remainText = remainPartsTextInfo->texts[0] + std::to_string(remainBoatPartsCount);
    mRenderer->DrawTextForElement(
        "default",
        "remainPartsText",
        mRenderer->GetFbWidth() -
            mRenderer->GetFbWidth() * remainPartsTextInfo->xRatio,
        mRenderer->GetFbWidth() * remainPartsTextInfo->yRatio,
        mRenderer->GetFbWidth() * remainPartsTextInfo->scaleRatio,
        remainText,
        remainPartsTextInfo->centerBased,
        {255, 255, 255, 255},
        remainPartsTextInfo->rotationDegrees);
}

void HudRenderer::DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float uiScale)
{
    mRenderer->DrawTextDependsOnPlayerInput(player, "state", "recommendReduceTiredText", screenTopY, uiScale);
}
