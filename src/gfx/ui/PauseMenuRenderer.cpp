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

        const bool selected = selectedIndex == i;

        std::string label = textInfo->texts[0];
        if (menuTextIds[i] == "controlStyleText") {
            label += mGame->IsAssistControlStyle() ? "アシスト" : "スタンダード";
        }

        std::string text = selected ? "> " : "  ";
        text += label;

        const glm::vec4 color = selected ? glm::vec4(255, 230, 0, 255) : glm::vec4(255, 255, 255, 255);

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
