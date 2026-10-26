#pragma once

#include "gfx/ui/PauseOperationGuideRenderer.h"

class Game;
class UIRenderer;

// ポーズメニューの項目・選択状態・操作案内を描画する。GameとUIRendererは非所有参照。
class PauseMenuRenderer {
public:
    PauseMenuRenderer(Game* game, UIRenderer* renderer);

    void Draw();

private:
    Game* mGame;
    UIRenderer* mRenderer;
    PauseOperationGuideRenderer mOperationGuideRenderer;
};
