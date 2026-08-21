#include "gfx/ui/SceneUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include "system/UILoadSystem.h"

SceneUIRenderer::SceneUIRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void SceneUIRenderer::DrawTitle()
{
    mRenderer->DrawSceneTexture("title", "bgTexture", "titleBg");
}

void SceneUIRenderer::DrawOpening()
{
    if (mGame->GetSceneSystem()->IsTalkWithOpening()) {
        DrawOpeningIntro();
    } else if (mGame->GetSceneSystem()->IsTalkWithMother()) {
        DrawOpeningTalkWithMother();
    } else if (mGame->GetSceneSystem()->IsTalkWithDoctor()) {
        DrawOpeningTalkWithDoctor();
    }

}

void SceneUIRenderer::DrawEnding()
{
    mRenderer->DrawSceneTexture("ending", "bgTexture", "ending");
    if (mRenderer->DrawSceneTalkUI("ending", "endingText")) {
        return;
    }
    mGame->GetSceneSystem()->FinishEndingStory();
}

void SceneUIRenderer::DrawCredits()
{
    mRenderer->DrawSceneTexture("credits", "bgTexture", "credits");
    const UILoadSystem::TextInfo* textInfo =
        mRenderer->GetUILoadSystem()->GetTextInfo("credits", "creditsText");
    if (!textInfo || textInfo->texts.empty()) {
        return;
    }

    const float elapsed =
        mGame->GetSceneSystem()->GetCreditsElapsed();
    const float scrollY =
        mRenderer->GetFbHeight() * (1.15f - elapsed * 0.055f);
    mRenderer->DrawText(
        mRenderer->GetFbWidth() * textInfo->xRatio,
        scrollY,
        mRenderer->GetFbWidth() * textInfo->scaleRatio,
        textInfo->texts.front(),
        textInfo->centerBased,
        {255.0f, 255.0f, 255.0f, 255.0f},
        textInfo->rotationDegrees);
}

void SceneUIRenderer::DrawGameOver()
{
    mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(), {0.0f, 0.0f, 0.0f, 0.5f});
    mRenderer->DrawSceneText("gameOver", "gameOverText", 0);
    mRenderer->DrawTextDependsOnGameController("gameOver", "restartText");
}

void SceneUIRenderer::DrawOpeningIntro()
{
    mRenderer->DrawSceneTexture("opening", "bgTexture", "opening");
    if (mRenderer->DrawSceneTalkUI("opening", "openingText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Mother);
}

void SceneUIRenderer::DrawOpeningTalkWithMother()
{
    if (mRenderer->DrawSceneTalkUI("opening", "talkWithMotherText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Doctor);
}

void SceneUIRenderer::DrawOpeningTalkWithDoctor()
{
    const UILoadSystem::TextInfo* talkWithDoctorTextInfo =
        mRenderer->GetUILoadSystem()->GetTextInfo("opening", "talkWithDoctorText");

    if (!talkWithDoctorTextInfo) {
        return;
    }

    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();
    const std::vector<std::string>& talkTexts = talkWithDoctorTextInfo->texts;
    const bool isTalking = talkUIIndex >= 0 && talkUIIndex < static_cast<int>(talkTexts.size());
    if (isTalking) {
        mRenderer->DrawTalkUI(talkWithDoctorTextInfo);
        return;
    }

    const bool isFinishTalk = talkUIIndex >= static_cast<int>(talkTexts.size());
    if (isFinishTalk) {
        mGame->GetSceneSystem()->FinishOpeningStory();
    }
}
