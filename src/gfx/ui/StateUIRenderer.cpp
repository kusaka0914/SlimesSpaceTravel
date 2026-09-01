#include "gfx/ui/StateUIRenderer.h"

#include "gfx/UIRenderer.h"
#include "gfx/video/TutorialVideoPlayer.h"
#include "Game.h"
#include "actor/NPC.h"
#include "state/UIState.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/tutorial/TutorialLibrary.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

StateUIRenderer::StateUIRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer),
      mTutorialVideoPlayer(
          std::make_unique<TutorialVideoPlayer>())
{
}

StateUIRenderer::~StateUIRenderer() = default;

void StateUIRenderer::DrawStateUI()
{
    if (mGame->GetSceneSystem()->IsShowingTutorialConversation()) {
        DrawActiveTutorial();
    } else if (mGame->GetSceneSystem()->IsShowingTutorialObjective()) {
        mTutorialVideoPlayer->Stop();
        DrawActionObjective();
    } else {
        mTutorialVideoPlayer->Stop();
    }

    if (mGame->GetSceneSystem()->IsTalkWithNPC()) {
        DrawTalkWithNPC();
    }

    if (mGame->GetSceneSystem()->IsStageClear()) {
        DrawStageClear();
    }
}

void StateUIRenderer::DrawActionObjective()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    TutorialController* tutorialController =
        sceneSystem ? sceneSystem->GetTutorialController() : nullptr;
    const TutorialPage* page =
        tutorialController
            ? tutorialController->GetCurrentPage()
            : nullptr;
    if (!page) {
        return;
    }

    const std::string& objectiveText =
        page->ResolveObjectiveText(
            mGame->IsGameControllerConnected());
    if (objectiveText.empty()) {
        return;
    }
    std::string singleLineObjectiveText = objectiveText;
    std::replace(
        singleLineObjectiveText.begin(),
        singleLineObjectiveText.end(),
        '\n',
        ' ');

    const float framebufferWidth =
        static_cast<float>(mRenderer->GetFbWidth());
    const float framebufferHeight =
        static_cast<float>(mRenderer->GetFbHeight());
    const float textScale = framebufferWidth * 0.00022f;
    int textWidth = 0;
    int textHeight = 0;
    if (!mRenderer->CalculateTextSize(
            singleLineObjectiveText,
            textScale,
            textWidth,
            textHeight) ||
        textHeight <= 0) {
        return;
    }

    const float rightMargin = framebufferWidth * 0.02f;
    const float topMargin = framebufferHeight * 0.025f;
    const float textLeft = std::max(
        rightMargin,
        framebufferWidth - rightMargin -
            static_cast<float>(textWidth));
    UIRenderer::TextEffect textEffect;
    textEffect.shadowEnabled = true;
    textEffect.shadowOffset = glm::vec2(2.0f, 2.0f);
    textEffect.shadowColor =
        glm::vec4(0.0f, 0.0f, 0.0f, 220.0f);
    mRenderer->DrawText(
        textLeft,
        topMargin,
        textScale,
        singleLineObjectiveText,
        false,
        {255.0f, 255.0f, 255.0f, 255.0f},
        0.0f,
        &textEffect);
}

void StateUIRenderer::DrawTransitionUI()
{
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

void StateUIRenderer::DrawTalkWithNPC()
{
    NPC* talkingNPC = mGame->GetSceneSystem()->GetTalkingNPC();
    if (!talkingNPC) {
        return;
    }

    const std::vector<std::string> talkTexts =
        talkingNPC->GetResolvedTalkTexts();
    const int talkUIIndex = mGame->GetSceneSystem()->GetTalkUIIndex();

    if (talkUIIndex < 0 ||
        talkUIIndex >= static_cast<int>(talkTexts.size())) {
        return;
    }

    const std::vector<RubyTextSegment>& rubySegments =
        talkingNPC->GetResolvedTalkRubySegments(
            static_cast<std::size_t>(talkUIIndex));
    mRenderer->DrawTalkUI(talkTexts, talkUIIndex, &rubySegments);
}

void StateUIRenderer::DrawActiveTutorial()
{
    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    TutorialController* tutorialController =
        sceneSystem ? sceneSystem->GetTutorialController() : nullptr;
    const TutorialDefinition* definition =
        tutorialController
            ? tutorialController->GetActiveDefinition()
            : nullptr;
    if (!definition) {
        return;
    }

    const bool usesController =
        mGame->IsGameControllerConnected();
    const std::vector<TutorialPage>& pages =
        definition->GetPagesForControlStyle(
            mGame->IsAssistControlStyle());
    UILoadSystem::TextInfo textInfo;
    textInfo.xRatio = definition->textXRatio;
    textInfo.yRatio = definition->textYRatio;
    textInfo.scaleRatio = definition->textScaleRatio;
    textInfo.texts.reserve(pages.size());
    textInfo.rubySegments.reserve(pages.size());

    for (const TutorialPage& page : pages) {
        textInfo.texts.emplace_back(
            page.ResolveText(usesController));
        textInfo.rubySegments.emplace_back(
            page.ResolveRubySegments(usesController));
    }

    mRenderer->DrawTalkUI(&textInfo);

    const TutorialPage* currentPage =
        tutorialController->GetCurrentPage();
    if (!currentPage) {
        mTutorialVideoPlayer->Stop();
        return;
    }

    const std::string playbackKey =
        definition->id + ":" +
        std::to_string(
            tutorialController->GetTutorialSessionSequence()) +
        ":" + currentPage->video.assetPath;

    const int currentPageIndex = sceneSystem->GetTalkUIIndex();
    const TutorialVideoSettings* effectiveVideoSettings =
        &currentPage->video;
    for (int videoPageIndex = currentPageIndex;
         videoPageIndex > 0;
         --videoPageIndex) {
        const TutorialVideoSettings& currentVideoSettings =
            pages[
                static_cast<std::size_t>(videoPageIndex)]
                .video;
        const TutorialVideoSettings& previousVideoSettings =
            pages[
                static_cast<std::size_t>(videoPageIndex - 1)]
                .video;
        const bool usesSameVideoAsPreviousPage =
            currentVideoSettings.assetPath ==
                previousVideoSettings.assetPath &&
            previousVideoSettings.IsEnabled();
        if (!usesSameVideoAsPreviousPage) {
            break;
        }
        effectiveVideoSettings = &previousVideoSettings;
    }

    DrawActiveTutorialVideo(
        *effectiveVideoSettings,
        playbackKey);
}

void StateUIRenderer::DrawActiveTutorialVideo(
    const TutorialVideoSettings& videoSettings,
    const std::string& playbackKey)
{
    if (!videoSettings.IsEnabled()) {
        mTutorialVideoPlayer->Stop();
        return;
    }

    if (!mTutorialVideoPlayer->Play(
            playbackKey,
            videoSettings.assetPath,
            videoSettings.shouldLoop)) {
        return;
    }

    mTutorialVideoPlayer->Update();
    const GLuint textureHandle =
        mTutorialVideoPlayer->GetTextureHandle();
    if (textureHandle == 0) {
        return;
    }

    float x = mRenderer->GetFbWidth() * videoSettings.xRatio;
    float y = mRenderer->GetFbWidth() * videoSettings.yRatio;
    float width =
        mRenderer->GetFbWidth() * videoSettings.widthRatio;
    float height =
        mRenderer->GetFbWidth() * videoSettings.heightRatio;

    const int videoWidth =
        mTutorialVideoPlayer->GetVideoWidth();
    const int videoHeight =
        mTutorialVideoPlayer->GetVideoHeight();
    if (videoSettings.shouldPreserveAspectRatio &&
        videoWidth > 0 && videoHeight > 0 &&
        width > 0.0f && height > 0.0f) {
        const float widthScale =
            width / static_cast<float>(videoWidth);
        const float heightScale =
            height / static_cast<float>(videoHeight);
        const float fitScale = std::min(widthScale, heightScale);
        const float fittedWidth =
            static_cast<float>(videoWidth) * fitScale;
        const float fittedHeight =
            static_cast<float>(videoHeight) * fitScale;
        x += (width - fittedWidth) * 0.5f;
        y += (height - fittedHeight) * 0.5f;
        width = fittedWidth;
        height = fittedHeight;
    }

    mRenderer->DrawTextureHandle(
        x,
        y,
        width,
        height,
        textureHandle,
        videoSettings.shouldFlipVertical,
        videoSettings.rotationDegrees);
}

void StateUIRenderer::DrawStageClear()
{
    mRenderer->DrawSceneText("state", "stageClearText", 0);
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
    mRenderer->DrawSceneText("state", "loadingText", 0);
    mRenderer->DrawSceneTexture("state", "loadingTexture", "slime");
}
