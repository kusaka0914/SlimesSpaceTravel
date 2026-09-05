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

    const bool isTwoPlayer =
        mGame->GetIsPlayer2Joined() && players.size() >= 2;
    if (!isTwoPlayer) {
        DrawPlayerStatusUIForPlayer(mainPlayer, 0.0f, 1.0f);
    } else {
        const float halfHeight =
            static_cast<float>(mRenderer->GetFbHeight()) * 0.5f;
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

    const bool isTwoPlayer =
        mGame->GetIsPlayer2Joined() && players.size() >= 2;
    if (!isTwoPlayer) {
        DrawPlayerPromptUI(
            mGame->GetMainPlayer(),
            0.0f,
            static_cast<float>(mRenderer->GetFbHeight()));
        return;
    }

    const float halfHeight =
        static_cast<float>(mRenderer->GetFbHeight()) * 0.5f;
    DrawPlayerPromptUI(players[0], 0.0f, halfHeight);
    DrawPlayerPromptUI(players[1], halfHeight, halfHeight);
}

void HudRenderer::DrawPlayerStatusUIForPlayer(
    const Player* player,
    float screenTopY,
    float uiScale)
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

void HudRenderer::DrawSplitGuardUI(
    float screenTopY,
    float uiScale)
{
    const int maximumGuardCount =
        mGame->GetMaximumPlayerSplitGuardCount();
    if (maximumGuardCount <= 0) {
        return;
    }

    const int guardCount =
        mGame->GetPlayerSplitGuardCount();
    const float guardGap =
        mRenderer->GetFbWidth() * 0.025f;
    constexpr float depletedGuardOpacity = 0.2f;
    mRenderer->DrawLinedUpTextureSlots(
        "default",
        "splitGuardTexture",
        "guard",
        guardGap,
        guardCount,
        maximumGuardCount,
        depletedGuardOpacity,
        screenTopY,
        uiScale);
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

    const SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool hasModalConversation =
        sceneSystem &&
        (sceneSystem->IsTalkWithNPC() || sceneSystem->HasActiveTutorial());
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

void HudRenderer::DrawRecommendReduceTiredUI(const Player* player, float screenTopY, float screenHeight)
{
    mRenderer->DrawTextDependsOnPlayerInput(
        player,
        "state",
        "recommendReduceTiredText",
        screenTopY,
        screenHeight);
}

void HudRenderer::DrawUGCClearVerificationGuide()
{
    mRenderer->DrawSceneText(
        "state",
        "ugcClearVerificationGuideText",
        0,
        {255, 255, 255, 255});
}
