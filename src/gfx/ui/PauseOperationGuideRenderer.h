#pragma once

#include <string>

class Game;
class UIRenderer;

// ポーズ画面の右側に、操作方式・接続機器に対応する操作表を描く。参照先は所有しない。
class PauseOperationGuideRenderer {
public:
    PauseOperationGuideRenderer(Game& game, UIRenderer& renderer);
    void Draw();

private:
    void DrawButton(const std::string& elementId, bool usesController, float x, float y, float size, float opacity);
    void DrawFittedText(const std::string& text, float x, float y, float scale, float maximumWidth,
                        float opacity = 1.0f);

    Game& mGame;
    UIRenderer& mRenderer;
};
