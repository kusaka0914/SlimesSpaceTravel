#include "gfx/ui/PauseOperationGuideRenderer.h"

#include "app/Game.h"
#include "gfx/UIRenderer.h"

#include <algorithm>
#include <array>

namespace {
struct OperationRow {
    const char* buttonId;
    const char* gesture;
    const char* description;
    bool soloOnly = false;
};
} // namespace

PauseOperationGuideRenderer::PauseOperationGuideRenderer(Game& game, UIRenderer& renderer)
    : mGame(game),
      mRenderer(renderer)
{
}

void PauseOperationGuideRenderer::DrawFittedText(const std::string& text, float x, float y, float scale,
                                                 float maximumWidth, float opacity)
{
    int width = 0;
    int height = 0;
    if (mRenderer.CalculateTextSize(text, scale, width, height) && width > 0) {
        scale *= std::min(1.0f, maximumWidth / width);
    }
    UIRenderer::TextEffect effect;
    effect.outlineEnabled = true;
    effect.outlineWidth = 1.0f;
    effect.outlineColor = {0.0f, 0.0f, 0.0f, 255.0f * opacity};
    mRenderer.DrawText(x, y, scale, text, false, {255.0f, 255.0f, 255.0f, 255.0f * opacity}, 0.0f, &effect);
}

void PauseOperationGuideRenderer::DrawButton(const std::string& elementId, bool usesController, float x, float y,
                                             float size, float opacity)
{
    if (elementId.empty()) {
        return;
    }
    if (!usesController && (elementId == "buttonB_copy" || elementId == "buttonB_copy_copy")) {
        const std::array<const char*, 4> movementKeys = {"w", "a", "s", "d"};
        const std::array<const char*, 4> cameraKeys = {"arrow_up", "arrow_left", "arrow_down", "arrow_right"};
        const auto& keys = elementId == "buttonB_copy" ? movementKeys : cameraKeys;
        const float keySize = size * 0.48f;
        for (std::size_t index = 0; index < keys.size(); ++index) {
            const std::string path = std::string("textures/Inputs/Keyboard/keyboard_") + keys[index] + ".png";
            if (mRenderer.RegisterCustomUITexture(path)) {
                mRenderer.DrawTextureHandle(x + (index % 2) * size * 0.52f, y + (index / 2) * size * 0.52f, keySize,
                                            keySize, mRenderer.GetCustomUITextureHandle(path), false, 0.0f, opacity);
            }
        }
        return;
    }
    const auto* definitions = mRenderer.GetUILoadSystem();
    if (!definitions) {
        return;
    }
    for (const auto& element : definitions->GetCustomElements()) {
        if (element.screen != "operation" || element.id != elementId) {
            continue;
        }
        const std::string& variant = usesController ? element.gameControllerTexturePath : element.keyboardTexturePath;
        const std::string& path = element.usesInputDeviceVariants && !variant.empty() ? variant : element.texturePath;
        const bool flipVertical =
            element.usesInputDeviceVariants && !variant.empty()
                ? (usesController ? element.gameControllerFlipVertical : element.keyboardFlipVertical)
                : element.flipVertical;
        if (mRenderer.RegisterCustomUITexture(path)) {
            mRenderer.DrawTextureHandle(x, y, size, size, mRenderer.GetCustomUITextureHandle(path), flipVertical, 0.0f,
                                        opacity);
        }
        return;
    }
}

void PauseOperationGuideRenderer::Draw()
{
    const float width = static_cast<float>(mRenderer.GetFbWidth());
    const float height = static_cast<float>(mRenderer.GetFbHeight());
    if (width <= 0.0f || height <= 0.0f) {
        return;
    }
    const bool isTwoPlayer = mGame.GetIsPlayer2Joined();
    const bool assist = mGame.IsAssistControlStyle();
    // 1人プレイでは最後に使った機器、2人プレイでは各Playerに割り当てた機器を示す。
    const bool primaryController = isTwoPlayer ? mGame.HasGameControllerForPlayer(1)
                                               : mGame.IsGameControllerConnected() &&
                                                     mGame.GetLastUsedInputDevice() == InputDeviceType::GameController;
    const bool secondController = mGame.HasGameControllerForPlayer(2);
    const float scale = std::min(width * 0.00024f, height * 0.00043f);
    const float buttonSize = std::min(width * 0.026f, height * 0.044f);
    const float buttonX = width * 0.485f;
    const float secondButtonX = width * 0.54f;
    const float gestureX = width * (isTwoPlayer ? 0.625f : 0.590f);
    const float descriptionX = width * (isTwoPlayer ? 0.730f : 0.695f);
    const float gestureWidth = descriptionX - gestureX - width * 0.012f;
    const float descriptionWidth = width * 0.970f - descriptionX;
    const float rowHeight = height * 0.040f;
    float y = height * 0.19f;

    mRenderer.DrawBG(width * 0.465f, height * 0.07f, width * 0.515f, height * 0.86f, {0.0f, 0.0f, 0.0f, 0.80f}, 0.0f,
                     8.0f, 1.0f, {1.0f, 1.0f, 1.0f, 0.30f});
    DrawFittedText(assist ? "操作方法 — らくらくスタイル" : "操作方法 — こまかくスタイル", buttonX, height * 0.10f,
                   scale * 1.1f, width * 0.48f);
    DrawFittedText(isTwoPlayer ? "P1" : (primaryController ? "ボタン" : "キー"), buttonX, height * 0.15f, scale * 0.8f,
                   width * 0.13f);
    if (isTwoPlayer) {
        DrawFittedText("P2", secondButtonX, height * 0.15f, scale * 0.8f, width * 0.06f);
    }
    DrawFittedText("押し方", gestureX, height * 0.15f, scale * 0.8f, gestureWidth);
    DrawFittedText("操作内容", descriptionX, height * 0.15f, scale * 0.8f, descriptionWidth);

    const auto heading = [&](const char* label) {
        mRenderer.DrawBG(width * 0.475f, y, width * 0.49f, rowHeight * 0.75f, {1.0f, 1.0f, 1.0f, 0.07f});
        DrawFittedText(label, buttonX, y + rowHeight * 0.04f, scale * 0.88f, width * 0.48f);
        y += rowHeight;
    };
    const auto row = [&](const OperationRow& entry) {
        const float opacity = isTwoPlayer && entry.soloOnly ? 0.4f : 1.0f;
        DrawButton(entry.buttonId, primaryController, buttonX, y, buttonSize, opacity);
        if (isTwoPlayer) {
            DrawButton(entry.buttonId, secondController, secondButtonX, y, buttonSize, opacity);
        }
        DrawFittedText(entry.gesture, gestureX, y, scale * 0.84f, gestureWidth, opacity);
        DrawFittedText(entry.description, descriptionX, y, scale, descriptionWidth, opacity);
        y += rowHeight;
    };

    heading("基本操作");
    row({"buttonB_copy", "", "移動"});
    row({"buttonB_copy_copy", "", "カメラ回転"});
    row({"buttonA", "", "ジャンプ"});
    row({"buttonA", "空中で押す", "空中回避"});
    row({"buttonB", "", "回避"});

    heading("攻撃・回復");
    row({"buttonY", "", assist ? "攻撃" : "弱攻撃"});
    row({"buttonY", "長押し", "連続攻撃"});
    row({"buttonX", "", assist ? "HP回復" : "強攻撃"});
    row({"buttonX", "長押し", "溜め攻撃（離して発動）"});
    row({"buttonB_copy_copy2_copy2", "", "HP回復"});

    heading("その他");
    row({"buttonB_copy_copy2_copy", "", "分裂・合体", true});
    row({"buttonB_copy_copy2", "", "スライム切り替え", true});
    row({"buttonB_copy_copy_copy", "", "カメラリセット"});

    DrawButton("buttonA_copy", primaryController, buttonX, height * 0.87f, buttonSize, 1.0f);
    DrawFittedText("メニューを閉じる", buttonX + buttonSize * 1.4f, height * 0.87f, scale * 0.9f, width * 0.44f);
}
