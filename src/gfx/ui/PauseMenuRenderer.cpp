#include "gfx/ui/PauseMenuRenderer.h"

// GLFWがOpenGLヘッダを読み込む前にGLEWを読み込む。
#include <GL/glew.h>

#include "app/Game.h"
#include "gfx/UIRenderer.h"
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
#include <vector>

PauseMenuRenderer::PauseMenuRenderer(Game* game, UIRenderer* renderer)
    : mGame(game),
      mRenderer(renderer),
      mOperationGuideRenderer(*game, *renderer)
{
}

void PauseMenuRenderer::Draw()
{
    mRenderer->DrawBGFromUIInfo("pauseMenu", "overlayBg", {0.0f, 0.0f, 0.0f, 0.55f});
    const bool showsOperationTable = !mGame->GetIsUGCMode() && !mGame->GetIsUGCPlaytestActive();
    const float width = static_cast<float>(mRenderer->GetFbWidth());
    const float height = static_cast<float>(mRenderer->GetFbHeight());
    if (showsOperationTable) {
        mRenderer->DrawBG(width * 0.025f, height * 0.18f, width * 0.41f, height * 0.62f, {0.0f, 0.0f, 0.0f, 0.80f},
                          0.0f, 8.0f, 1.0f, {1.0f, 1.0f, 1.0f, 0.30f});
        mRenderer->DrawBG(width * 0.45f, height * 0.12f, std::max(1.0f, width * 0.001f), height * 0.76f,
                          {1.0f, 1.0f, 1.0f, 0.25f});
        mOperationGuideRenderer.Draw();
    } else {
        mRenderer->DrawBGFromUIInfo("pauseMenu", "panelBg", {0.0f, 0.0f, 0.0f, 0.75f});
        mRenderer->DrawSceneText("pauseMenu", "titleText", 0);
    }

    std::vector<std::string> menuTextIds = {
        "resumeText", "controlStyleText", "twoPlayerText", "returnBaseText", "feedbackText", "quitText",
    };

    const int selectedIndex = mGame->GetPauseMenuSelectedIndex();

    for (int menuIndex = 0; menuIndex < static_cast<int>(menuTextIds.size()); ++menuIndex) {
        const UILoadSystem::TextInfo* textInfo =
            mRenderer->GetUILoadSystem()->GetTextInfo("pauseMenu", menuTextIds[menuIndex]);
        if (!textInfo || textInfo->texts.empty()) {
            continue;
        }

        const bool isTwoPlayerEntry = menuTextIds[menuIndex] == "twoPlayerText";
        const bool isReturnBaseEntry = menuTextIds[menuIndex] == "returnBaseText";
        const bool isEnabled = (!isTwoPlayerEntry || mGame->CanStartTwoPlayerFromPauseMenu()) &&
                               (!isReturnBaseEntry || mGame->CanReturnToBaseFromPauseMenu());
        const bool isSelected = isEnabled && selectedIndex == menuIndex;

        std::string label = textInfo->texts[0];
        if (menuTextIds[menuIndex] == "feedbackText" && mGame->IsReviewBuild()) {
            label = mGame->GetIsDebugEditorShowing() ? "エディターを閉じる" : "エディターを開く";
        }
        if (menuTextIds[menuIndex] == "controlStyleText") {
            label += mGame->IsAssistControlStyle() ? "らくらく" : "こまかく";
        }
        if (isTwoPlayerEntry && mGame->GetIsPlayer2Joined()) {
            label = "ひとりであそぶ";
        } else if (isTwoPlayerEntry && !mGame->IsGameControllerConnected()) {
            label += "（コントローラーをつなぐ）";
        }

        std::string text = isSelected ? "> " : "  ";
        text += label;

        const glm::vec4 color = !isEnabled   ? glm::vec4(125, 125, 125, 255)
                                : isSelected ? glm::vec4(255, 230, 0, 255)
                                             : glm::vec4(255, 255, 255, 255);

        float textScale = width * textInfo->scaleRatio;
        if (showsOperationTable) {
            textScale = std::min(textScale, height * 0.00055f);
            int textWidth = 0;
            int textHeight = 0;
            if (mRenderer->CalculateTextSize(text, textScale, textWidth, textHeight) && textWidth > 0) {
                textScale *= std::min(1.0f, width * 0.38f / textWidth);
            }
        }
        mRenderer->DrawTextForElement(
            "pauseMenu", menuTextIds[menuIndex], width * (showsOperationTable ? 0.23f : textInfo->xRatio),
            height * textInfo->yRatio, textScale, text, textInfo->centerBased, color, textInfo->rotationDegrees);
    }

    if (!showsOperationTable) {
        mRenderer->DrawTextDependsOnGameController("pauseMenu", "operationText");
    }
}
