#pragma once

#include <GL/glew.h>

#include <array>

class Game;
class Renderer3D;

// 1人用・2人用のビューポートを切り替え、描画時間をGameへ記録する。GameとRenderer3Dは非所有参照。
// GPU計測用クエリを所有するため、GLコンテキストの破棄前にShutdownを呼ぶ。
class RenderViewportController {
public:
    RenderViewportController(Game* game, const Renderer3D* renderer);

    // fbWidth/fbHeightは描画先のピクセル寸法。2人時は上下分割し、ボス撃破演出中は1画面に戻す。
    void DrawGameScreen(float fbWidth, float fbHeight) const;

    // GPU計測用クエリを解放する。繰り返し呼び出せる。
    void Shutdown();

private:
    static constexpr int GpuTimerSlotCount = 3;

    void DrawGameScreenForSinglePerson(float fbWidth, float fbHeight) const;
    void DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const;
    void PollGpuTimerQueries() const;
    GLuint BeginGpuTimerQuery(int viewportIndex) const;
    void EndGpuTimerQuery(GLuint query) const;

private:
    Game* mGame;
    const Renderer3D* mRenderer;
    mutable std::array<std::array<GLuint, GpuTimerSlotCount>, 2> mGpuTimerQueries{};
    mutable std::array<std::array<bool, GpuTimerSlotCount>, 2> mIsGpuTimerQueryPending{};
    mutable std::array<int, 2> mNextGpuTimerSlot{};
    mutable bool mAreGpuTimerQueriesInitialized = false;
};
