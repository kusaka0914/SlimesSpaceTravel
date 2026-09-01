#include "gfx/ui/SceneUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "system/SceneSystem.h"
#include "system/UILoadSystem.h"
#include "system/ending/EndingRollConfig.h"

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
    mRenderer->DrawSceneTalkUI("ending", "endingText");
}

void SceneUIRenderer::DrawCredits()
{
    const EndingRollConfig& config =
        mGame->GetSceneSystem()->GetEndingRollConfig();
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

void SceneUIRenderer::DrawUGCClearResult()
{
    const float framebufferWidth =
        static_cast<float>(mRenderer->GetFbWidth());
    const float framebufferHeight =
        static_cast<float>(mRenderer->GetFbHeight());
    mRenderer->DrawBG(
        0.0f,
        0.0f,
        framebufferWidth,
        framebufferHeight,
        {0.0f, 0.0f, 0.0f, 0.62f});
    mRenderer->DrawSceneText("ugcClearResult", "titleText", 0);

    constexpr const char* resultItemIds[] = {
        "retryText",
        "returnEditorText",
        "returnTitleText",
    };
    const int selectedIndex = mGame->GetUGCClearResultSelection();
    for (int index = 0; index < 3; ++index) {
        const UILoadSystem::TextInfo* textInfo =
            mRenderer->GetUILoadSystem()->GetTextInfo(
                "ugcClearResult",
                resultItemIds[index]);
        if (!textInfo || textInfo->texts.empty()) {
            continue;
        }

        const bool isSelected = selectedIndex == index;
        const std::string label =
            (isSelected ? "> " : "  ") + textInfo->texts[0];
        mRenderer->DrawTextForElement(
            "ugcClearResult",
            resultItemIds[index],
            framebufferWidth * textInfo->xRatio,
            framebufferHeight * textInfo->yRatio,
            framebufferWidth * textInfo->scaleRatio,
            label,
            textInfo->centerBased,
            isSelected
                ? glm::vec4(255.0f, 230.0f, 0.0f, 255.0f)
                : glm::vec4(255.0f));
    }
}

void SceneUIRenderer::DrawOpeningIntro()
{
    DrawStorybookPage("opening", "opening", 0);
    mRenderer->DrawSceneTalkUI("opening", "openingText");
}

void SceneUIRenderer::DrawOpeningTalkWithMother()
{
    const UILoadSystem::TextInfo* intro =
        mRenderer->GetUILoadSystem()->GetTextInfo("opening", "openingText");
    DrawStorybookPage(
        "opening", "opening",
        intro ? static_cast<int>(intro->texts.size()) : 0);
    mRenderer->DrawSceneTalkUI("opening", "talkWithMotherText");
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
    mRenderer->DrawSceneTalkUI("opening", "talkWithDoctorText");
}

void SceneUIRenderer::DrawStorybookPage(
    const std::string& trackId,
    const std::string& fallbackScene,
    int pageOffset)
{
    const int pageIndex = pageOffset + mGame->GetSceneSystem()->GetTalkUIIndex();
    const std::string imagePath =
        mGame->GetSceneSystem()->FindStorybookPageImage(
            trackId,
            pageIndex);
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
