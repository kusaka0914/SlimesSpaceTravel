#include "gfx/ui/StateUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "actor/NPC.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include <string>
#include <vector>

StateUIRenderer::StateUIRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void StateUIRenderer::DrawStateUI()
{
    if (mGame->GetSceneSystem()->IsBattleTutorialShowing()) {
        DrawBattleTutorial();
    } else if (mGame->GetSceneSystem()->IsBreakTutorialShowing()) {
        DrawBreakTutorial();
    } else if (mGame->GetSceneSystem()->IsJewelTutorialShowing()) {
        DrawJewelTutorial();
    } else if (mGame->GetSceneSystem()->IsJustDodgeTutorialShowing()) {
        DrawJustDodgeTutorial();
    }

    if (mGame->GetSceneSystem()->IsTalkWithNPC()) {
        DrawTalkWithNPC();
    }

    if (mGame->GetSceneSystem()->IsStageClear()) {
        DrawStageClear();
    }

    const float alpha = CalculateAlpha();
    if (alpha > 0.0f) {
        DrawFadeInBg(alpha);
    }

    const bool isLoading =
        mGame->GetSceneSystem()->GetHasPendingStageChange() && mGame->GetSceneSystem()->GetFadeTimer() <= 0.1f;
    if (isLoading) {
        DrawLoading();
    }
}

void StateUIRenderer::DrawBattleTutorial()
{
    if (mRenderer->DrawSceneTalkUIDependsOnGameController("state", "battleTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void StateUIRenderer::DrawBreakTutorial()
{
    if (mRenderer->DrawSceneTalkUI("state", "breakTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void StateUIRenderer::DrawJewelTutorial()
{
    if (mRenderer->DrawSceneTalkUIDependsOnGameController("state", "jewelTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void StateUIRenderer::DrawJustDodgeTutorial()
{
    if (mRenderer->DrawSceneTalkUI("state", "justDodgeTutorialText")) {
        return;
    }

    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTutorial();
}

void StateUIRenderer::DrawTalkWithNPC()
{
    NPC* talkingNPC = mGame->GetSceneSystem()->GetTalkingNPC();
    if (!talkingNPC) {
        mGame->StartPlayingScene();
        mGame->GetSceneSystem()->GetUIState()->FinishTalkWith();
        return;
    }

    const std::vector<std::string> talkTexts =
        talkingNPC->GetResolvedTalkTexts();
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    const bool isTalking = talkUIIndex < static_cast<int>(talkTexts.size());
    if (isTalking) {
        const std::vector<RubyTextSegment>& rubySegments =
            talkingNPC->GetResolvedTalkRubySegments(
                static_cast<std::size_t>(talkUIIndex));
        mRenderer->DrawTalkUI(talkTexts, talkUIIndex, &rubySegments);
        return;
    }

    talkingNPC->MarkTalkCompletedThisVisit();
    mGame->StartPlayingScene();
    mGame->GetSceneSystem()->GetUIState()->FinishTalkWith();
}

void StateUIRenderer::DrawStageClear()
{
    mRenderer->DrawSceneText("state", "stageClearText", true, 0);
}

float StateUIRenderer::CalculateAlpha() const
{
    const float fadeInTimer = mGame->GetSceneSystem()->GetFadeTimer();
    if (fadeInTimer >= 0.0f) {
        return 1.0f - fadeInTimer;
    }
    return 1.0f + fadeInTimer;
}

void StateUIRenderer::DrawFadeInBg(float alpha)
{
    mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(), {0.0f, 0.0f, 0.0f, alpha});
}

void StateUIRenderer::DrawLoading()
{
    mRenderer->DrawSceneText("state", "loadingText", false, 0);
    mRenderer->DrawSceneTexture("state", "loadingTexture", "slime");
}
