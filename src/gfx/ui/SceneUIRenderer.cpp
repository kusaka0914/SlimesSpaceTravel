#include "gfx/ui/SceneUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"

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
        mGame->GetSceneSystem()->StartFadeIn();
    }
}
