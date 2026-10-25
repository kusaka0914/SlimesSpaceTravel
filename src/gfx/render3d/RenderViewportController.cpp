#include "gfx/render3d/RenderViewportController.h"

#include "gfx/Renderer3D.h"
#include "gfx/performance/FramePerformanceTracker.h"
#include "gfx/render3d/SceneRenderState.h"
#include "system/CameraSystem.h"
#include "system/camera/CameraProjection.h"

#include <GL/glew.h>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

RenderViewportController::RenderViewportController(CameraSystem& cameras, FramePerformanceTracker& performance,
                                                   const Renderer3D* renderer)
    : mCameras(cameras),
      mPerformance(performance),
      mRenderer(renderer)
{
}

void RenderViewportController::DrawGameScreen(float fbWidth, float fbHeight, const SceneRenderState& scene) const
{
    if (!mRenderer || fbWidth <= 0.0f || fbHeight <= 0.0f) {
        return;
    }

    PollGpuTimerQueries();

    if (!scene.isPlayer2Joined) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight, scene);
        return;
    }

    DrawGameScreenForMultiPerson(fbWidth, fbHeight, scene);
}

void RenderViewportController::DrawGameScreenForSinglePerson(float fbWidth, float fbHeight,
                                                             const SceneRenderState& scene) const
{
    glViewport(0, 0, static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(fbHeight));

    CameraSystem& cameraSystem = mCameras;
    if (fbHeight <= 0.0f) {
        return;
    }

    const float aspect = fbWidth / fbHeight;
    const float fieldOfViewDegrees = cameraSystem.GetFieldOfViewDegrees();
    const glm::mat4 projMat = CalculateCameraProjection(scene.projection, aspect, fieldOfViewDegrees);

    std::vector<glm::mat4> views = cameraSystem.GetViews();
    if (views.empty()) {
        return;
    }

    const glm::vec3 cameraPosition = cameraSystem.GetCameraPos();
    const auto renderStartTime = std::chrono::steady_clock::now();
    const GLuint gpuTimerQuery = BeginGpuTimerQuery(0);
    mRenderer->DrawScene(views[0], projMat, cameraPosition, UGCSceneLayerRenderMode::AutomaticallyHighlightEditingLayer,
                         0, scene.controlledPlayer);
    EndGpuTimerQuery(gpuTimerQuery);
    mPerformance.RecordViewportCpuDuration(
        0, std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - renderStartTime).count());
}

void RenderViewportController::DrawGameScreenForMultiPerson(float fbWidth, float fbHeight,
                                                            const SceneRenderState& scene) const
{
    if (mCameras.IsBossDefeatSequencePlaying()) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight, scene);
        return;
    }

    std::vector<glm::mat4> views = mCameras.GetViews();

    if (views.size() < 2) {
        DrawGameScreenForSinglePerson(fbWidth, fbHeight, scene);
        return;
    }

    const float halfHeight = fbHeight * 0.5f;
    const float aspect = fbWidth / halfHeight;
    const float fieldOfViewDegrees = mCameras.GetFieldOfViewDegrees();
    const glm::mat4 projMat = glm::perspective(glm::radians(fieldOfViewDegrees), aspect, 0.1f, 100.0f);

    const glm::vec3 firstPlayerCameraPosition = mCameras.GetPlayerCameraPos(0);
    const glm::vec3 secondPlayerCameraPosition = mCameras.GetPlayerCameraPos(1);

    glViewport(0, static_cast<GLint>(halfHeight), static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    const auto& players = scene.players;
    const Player* player1 = players.empty() ? nullptr : players[0];
    const Player* player2 = players.size() < 2 ? nullptr : players[1];

    const auto firstViewportRenderStartTime = std::chrono::steady_clock::now();
    const GLuint firstViewportGpuTimerQuery = BeginGpuTimerQuery(0);
    mRenderer->DrawScene(views[0], projMat, firstPlayerCameraPosition,
                         UGCSceneLayerRenderMode::AutomaticallyHighlightEditingLayer, 0, player1);
    EndGpuTimerQuery(firstViewportGpuTimerQuery);
    mPerformance.RecordViewportCpuDuration(
        0, std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - firstViewportRenderStartTime)
               .count());

    glViewport(0, 0, static_cast<GLsizei>(fbWidth), static_cast<GLsizei>(halfHeight));
    const auto secondViewportRenderStartTime = std::chrono::steady_clock::now();
    const GLuint secondViewportGpuTimerQuery = BeginGpuTimerQuery(1);
    mRenderer->DrawScene(views[1], projMat, secondPlayerCameraPosition,
                         UGCSceneLayerRenderMode::AutomaticallyHighlightEditingLayer, 0, player2);
    EndGpuTimerQuery(secondViewportGpuTimerQuery);
    mPerformance.RecordViewportCpuDuration(
        1, std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - secondViewportRenderStartTime)
               .count());
}

void RenderViewportController::Shutdown()
{
    if (!mAreGpuTimerQueriesInitialized) {
        return;
    }

    for (const std::array<GLuint, GpuTimerSlotCount>& viewportQueries : mGpuTimerQueries) {
        glDeleteQueries(static_cast<GLsizei>(viewportQueries.size()), viewportQueries.data());
    }

    mGpuTimerQueries = {};
    mIsGpuTimerQueryPending = {};
    mNextGpuTimerSlot = {};
    mAreGpuTimerQueriesInitialized = false;
}

void RenderViewportController::PollGpuTimerQueries() const
{
    if (!mAreGpuTimerQueriesInitialized) {
        return;
    }

    for (int viewportIndex = 0; viewportIndex < 2; ++viewportIndex) {
        for (int slotIndex = 0; slotIndex < GpuTimerSlotCount; ++slotIndex) {
            if (!mIsGpuTimerQueryPending[viewportIndex][slotIndex]) {
                continue;
            }

            const GLuint query = mGpuTimerQueries[viewportIndex][slotIndex];
            GLint isResultAvailable = GL_FALSE;
            glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &isResultAvailable);
            if (isResultAvailable != GL_TRUE) {
                continue;
            }

            GLuint64 elapsedNanoseconds = 0;
            glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsedNanoseconds);
            const float elapsedMilliseconds = static_cast<float>(elapsedNanoseconds) / 1000000.0f;
            mPerformance.RecordViewportGpuDuration(viewportIndex, elapsedMilliseconds);
            mIsGpuTimerQueryPending[viewportIndex][slotIndex] = false;
        }
    }
}

GLuint RenderViewportController::BeginGpuTimerQuery(int viewportIndex) const
{
    if (viewportIndex < 0 || viewportIndex >= 2 || !GLEW_ARB_timer_query) {
        return 0;
    }

    if (!mAreGpuTimerQueriesInitialized) {
        for (std::array<GLuint, GpuTimerSlotCount>& viewportQueries : mGpuTimerQueries) {
            glGenQueries(static_cast<GLsizei>(viewportQueries.size()), viewportQueries.data());
        }
        mAreGpuTimerQueriesInitialized = true;
    }

    for (int attempt = 0; attempt < GpuTimerSlotCount; ++attempt) {
        const int slotIndex = (mNextGpuTimerSlot[viewportIndex] + attempt) % GpuTimerSlotCount;
        if (mIsGpuTimerQueryPending[viewportIndex][slotIndex]) {
            continue;
        }

        const GLuint query = mGpuTimerQueries[viewportIndex][slotIndex];
        glBeginQuery(GL_TIME_ELAPSED, query);
        mIsGpuTimerQueryPending[viewportIndex][slotIndex] = true;
        mNextGpuTimerSlot[viewportIndex] = (slotIndex + 1) % GpuTimerSlotCount;
        return query;
    }

    return 0;
}

void RenderViewportController::EndGpuTimerQuery(GLuint query) const
{
    if (query != 0) {
        glEndQuery(GL_TIME_ELAPSED);
    }
}
