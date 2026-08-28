#include "gfx/ui/SceneUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include "system/UILoadSystem.h"
#include "system/ending/EndingRollConfig.h"
#include "system/story/StorybookConfig.h"

#include <algorithm>
#include <sstream>

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
    DrawStorybookPage("ending", "ending");
    if (mRenderer->DrawSceneTalkUI("ending", "endingText")) {
        return;
    }
    mGame->GetSceneSystem()->FinishEndingStory();
}

void SceneUIRenderer::DrawCredits()
{
    EndingRollConfig config;
    EndingRollConfigIO::Load(config);
    const float elapsed = mGame->GetSceneSystem()->GetCreditsElapsed();
    const float width = static_cast<float>(mRenderer->GetFbWidth());
    const float height = static_cast<float>(mRenderer->GetFbHeight());
    mRenderer->DrawBG(0.0f, 0.0f, width, height, {0.0f, 0.0f, 0.0f, 1.0f});

    if (elapsed >= config.endImageStartTime && !config.endImagePath.empty() &&
        mRenderer->RegisterCustomUITexture(config.endImagePath)) {
        const float endImageElapsed = elapsed - config.endImageStartTime;
        const float endImageOpacity = config.endImageFadeInDuration > 0.0f
            ? std::clamp(endImageElapsed / config.endImageFadeInDuration, 0.0f, 1.0f)
            : 1.0f;
        mRenderer->DrawTextureHandle(0.0f, 0.0f, width, height,
                                     mRenderer->GetCustomUITextureHandle(config.endImagePath), true,
                                     0.0f, endImageOpacity);
        return;
    }

    for (const EndingRollImageEvent& event : config.imageEvents) {
        if (!IsEndingRollImageVisible(event, elapsed) ||
            !mRenderer->RegisterCustomUITexture(event.imagePath)) {
            continue;
        }
        mRenderer->DrawTextureHandle(
            width * event.xRatio - width * event.widthRatio * 0.5f,
            height * event.yRatio - height * event.heightRatio * 0.5f,
            width * event.widthRatio, height * event.heightRatio,
            mRenderer->GetCustomUITextureHandle(event.imagePath), true, 0.0f,
            CalculateEndingRollImageOpacity(event, elapsed));
    }

    const float firstLineY = height *
        (config.creditsStartYRatio - std::max(0.0f, elapsed - config.creditsStartTime) * config.creditsScrollSpeedRatio);
    std::istringstream lines(config.creditsText);
    std::string line;
    int lineIndex = 0;



    const float lineHeight = height * 0.0666f;
    while (std::getline(lines, line)) {
        mRenderer->DrawText(width * 0.5f, firstLineY + lineIndex * lineHeight,
                            width * config.creditsTextScaleRatio, line, true);
        ++lineIndex;
    }
}

void SceneUIRenderer::DrawGameOver()
{
    mRenderer->DrawBG(0.0f, 0.0f, mRenderer->GetFbWidth(), mRenderer->GetFbHeight(), {0.0f, 0.0f, 0.0f, 0.5f});
    mRenderer->DrawSceneText("gameOver", "gameOverText", 0);
    mRenderer->DrawSceneText("gameOver", "restartText", 0);
}

void SceneUIRenderer::DrawOpeningIntro()
{
    DrawStorybookPage("opening", "opening", 0);
    if (mRenderer->DrawSceneTalkUI("opening", "openingText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Mother);
}

void SceneUIRenderer::DrawOpeningTalkWithMother()
{
    const UILoadSystem::TextInfo* intro =
        mRenderer->GetUILoadSystem()->GetTextInfo("opening", "openingText");
    DrawStorybookPage(
        "opening", "opening",
        intro ? static_cast<int>(intro->texts.size()) : 0);
    if (mRenderer->DrawSceneTalkUI("opening", "talkWithMotherText")) {
        return;
    }

    mGame->GetSceneSystem()->GetUIState()->StartTalkWith(UIState::TalkWith::Doctor);
}

void SceneUIRenderer::DrawOpeningTalkWithDoctor()
{
    const UILoadSystem::TextInfo* intro =
        mRenderer->GetUILoadSystem()->GetTextInfo("opening", "openingText");
    const UILoadSystem::TextInfo* mother =
        mRenderer->GetUILoadSystem()->GetTextInfo("opening", "talkWithMotherText");
    const int pageOffset =
        (intro ? static_cast<int>(intro->texts.size()) : 0) +
        (mother ? static_cast<int>(mother->texts.size()) : 0);
    DrawStorybookPage("opening", "opening", pageOffset);
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

void SceneUIRenderer::DrawStorybookPage(
    const std::string& trackId,
    const std::string& fallbackScene,
    int pageOffset)
{
    StorybookConfig config;
    config.Load();
    const int pageIndex = pageOffset + mGame->GetSceneSystem()->GetTalkUIIndex();
    const std::string imagePath = config.GetPageImage(trackId, pageIndex);
    if (!imagePath.empty() && mRenderer->RegisterCustomUITexture(imagePath)) {
        mRenderer->DrawTextureHandle(
            0.0f, 0.0f,
            static_cast<float>(mRenderer->GetFbWidth()),
            static_cast<float>(mRenderer->GetFbHeight()),
            mRenderer->GetCustomUITextureHandle(imagePath), true);
        return;
    }
    mRenderer->DrawSceneTexture(fallbackScene, "bgTexture", fallbackScene);
}
