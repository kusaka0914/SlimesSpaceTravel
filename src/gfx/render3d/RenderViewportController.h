#pragma once

#include <GL/glew.h>

#include <array>

class Game;
class Renderer3D;

class RenderViewportController {
public:
    RenderViewportController(Game* game, const Renderer3D* renderer);

    void DrawGameScreen(float fbWidth, float fbHeight) const;
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
    mutable std::array<std::array<GLuint, GpuTimerSlotCount>, 2>
        mGpuTimerQueries{};
    mutable std::array<std::array<bool, GpuTimerSlotCount>, 2>
        mIsGpuTimerQueryPending{};
    mutable std::array<int, 2> mNextGpuTimerSlot{};
    mutable bool mAreGpuTimerQueriesInitialized = false;
};
