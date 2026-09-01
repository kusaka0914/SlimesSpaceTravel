#include "gfx/ui/PauseMenuRenderer.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>

PauseMenuRenderer::PauseMenuRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void PauseMenuRenderer::Draw()
{
    mRenderer->DrawBGFromUIInfo("pauseMenu", "overlayBg", {0.0f, 0.0f, 0.0f, 0.55f});
    mRenderer->DrawBGFromUIInfo("pauseMenu", "panelBg", {0.0f, 0.0f, 0.0f, 0.75f});

    mRenderer->DrawSceneText("pauseMenu", "titleText", 0);

    std::vector<std::string> menuTextIds = {
        "resumeText",
        "controlStyleText",
        "twoPlayerText",
        "returnBaseText",
        "feedbackText",
        "quitText",
    };

    const int selectedIndex = mGame->GetPauseMenuSelectedIndex();

    for (int i = 0; i < static_cast<int>(menuTextIds.size()); ++i) {
        const UILoadSystem::TextInfo* textInfo =
            mRenderer->GetUILoadSystem()->GetTextInfo("pauseMenu", menuTextIds[i]);
        if (!textInfo || textInfo->texts.empty()) {
            continue;
        }

        const bool isTwoPlayerEntry = menuTextIds[i] == "twoPlayerText";
        const bool isReturnBaseEntry = menuTextIds[i] == "returnBaseText";
        const bool enabled =
            (!isTwoPlayerEntry || mGame->CanStartTwoPlayerFromPauseMenu()) &&
            (!isReturnBaseEntry || mGame->CanReturnToBaseFromPauseMenu());
        const bool selected = enabled && selectedIndex == i;

        std::string label = textInfo->texts[0];
        if (menuTextIds[i] == "feedbackText" && mGame->IsReviewBuild()) {
            label = mGame->GetIsDebugEditorShowing()
                ? "エディターを閉じる"
                : "エディターを開く";
        }
        if (menuTextIds[i] == "controlStyleText") {
            label += mGame->IsAssistControlStyle() ? "アシスト" : "スタンダード";
        }
        if (isTwoPlayerEntry && mGame->GetIsPlayer2Joined()) {
            label = "ひとりであそぶ";
        } else if (isTwoPlayerEntry && !mGame->IsGameControllerConnected()) {
            label += "（コントローラーをつなぐ）";
        }

        std::string text = selected ? "> " : "  ";
        text += label;

        const glm::vec4 color = !enabled
                                    ? glm::vec4(125, 125, 125, 255)
                                    : selected
                                          ? glm::vec4(255, 230, 0, 255)
                                          : glm::vec4(255, 255, 255, 255);

        mRenderer->DrawTextForElement(
            "pauseMenu",
            menuTextIds[i],
            mRenderer->GetFbWidth() * textInfo->xRatio,
            mRenderer->GetFbHeight() * textInfo->yRatio,
            mRenderer->GetFbWidth() * textInfo->scaleRatio,
            text,
            textInfo->centerBased,
            color,
            textInfo->rotationDegrees);
    }

    mRenderer->DrawTextDependsOnGameController("pauseMenu", "operationText");
}
