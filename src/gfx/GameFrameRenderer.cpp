#include <GL/glew.h>

#include "gfx/GameFrameRenderer.h"

#include "gfx/Renderer3D.h"
#include "gfx/PostProcessRenderer.h"
#include "gfx/UIRenderer.h"
#include "gfx/debug/ugc/UGCPreviewController.h"
#include "gfx/performance/GpuDurationTimer.h"
#include "gfx/performance/FramePerformanceTracker.h"
#include "system/CameraSystem.h"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <cmath>
#include <optional>

GameFrameRenderer::GameFrameRenderer(
    GLFWwindow& window,
    Renderer3D& renderer3D,
    UIRenderer& uiRenderer,
    CameraSystem& cameraSystem,
    UGCPreviewController& ugcPreviewController,
    FramePerformanceTracker& performanceTracker)
    : mWindow(window),
      mRenderer3D(renderer3D),
      mUIRenderer(uiRenderer),
      mCameraSystem(cameraSystem),
      mUGCPreviewController(ugcPreviewController),
      mPerformanceTracker(performanceTracker),
      mGamePostProcessRenderer(std::make_unique<PostProcessRenderer>()),
      mUGCPreviewPostProcessRenderer(
          std::make_unique<PostProcessRenderer>()),
      mGameUiGpuTimer(std::make_unique<GpuDurationTimer>()),
      mEditorUiGpuTimer(std::make_unique<GpuDurationTimer>())
{
}

GameFrameRenderer::~GameFrameRenderer() = default;

void GameFrameRenderer::Render(
    const GameFrameRenderState& renderState)
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(
        &mWindow,
        &framebufferWidth,
        &framebufferHeight);

    const bool shouldRenderEditorGameView =
        renderState.isDebugEditorShowing &&
        mRenderTargets.EnsureEditorGameTarget(
            framebufferWidth, framebufferHeight);
    if (shouldRenderEditorGameView) {
        DrawGameFrame(
            mRenderTargets.GetEditorGameFramebuffer(),
            framebufferWidth,
            framebufferHeight);

        const int previewWidth =
            mUGCPreviewController.GetRenderWidth();
        const int previewHeight =
            mUGCPreviewController.GetRenderHeight();
        if (renderState.isUGCModeActive &&
            mRenderTargets.EnsureUGCPreviewTarget(
                previewWidth, previewHeight)) {
            glBindFramebuffer(
                GL_FRAMEBUFFER,
                mRenderTargets.GetUGCPreviewFramebuffer());
            DrawUGCPreviewFrame(renderState);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClearColor(0.035f, 0.035f, 0.045f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto editorUiStartTime = std::chrono::steady_clock::now();
        mEditorUiGpuTimer->Begin();
        mUIRenderer.DrawDebugEditor(
            mRenderTargets.GetEditorGameTexture(),
            framebufferWidth,
            framebufferHeight);
        mEditorUiGpuTimer->End();
        mPerformanceTracker.RecordEditorUiCpuDuration(
            std::chrono::duration<float, std::milli>(
                std::chrono::steady_clock::now() - editorUiStartTime).count());
    } else {
        DrawGameFrame(0, framebufferWidth, framebufferHeight);
        if (renderState.isDebugEditorShowing) {
            const auto editorUiStartTime = std::chrono::steady_clock::now();
            mEditorUiGpuTimer->Begin();
            mUIRenderer.DrawDebugEditor(
                0,
                framebufferWidth,
                framebufferHeight);
            mEditorUiGpuTimer->End();
            mPerformanceTracker.RecordEditorUiCpuDuration(
                std::chrono::duration<float, std::milli>(
                    std::chrono::steady_clock::now() - editorUiStartTime)
                    .count());
        } else if (renderState.isUGCWorkBrowserShowing) {
            mUIRenderer.DrawUGCWorkBrowser();
        }
    }

    const auto presentationStartTime = std::chrono::steady_clock::now();
    glfwSwapBuffers(&mWindow);
    mPerformanceTracker.RecordPresentationWaitDuration(
        std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - presentationStartTime).count());
}

void GameFrameRenderer::PollGpuPerformanceMeasurements()
{
    if (mGameUiGpuTimer) {
        const std::optional<float> elapsedMilliseconds =
            mGameUiGpuTimer->PollCompletedMilliseconds();
        if (elapsedMilliseconds) {
            mPerformanceTracker.RecordGameUiGpuDuration(
                *elapsedMilliseconds);
        }
    }

    if (mEditorUiGpuTimer) {
        const std::optional<float> elapsedMilliseconds =
            mEditorUiGpuTimer->PollCompletedMilliseconds();
        if (elapsedMilliseconds) {
            mPerformanceTracker.RecordEditorUiGpuDuration(
                *elapsedMilliseconds);
        }
    }
}

void GameFrameRenderer::Shutdown()
{
    if (mGameUiGpuTimer) {
        mGameUiGpuTimer->Shutdown();
    }
    if (mEditorUiGpuTimer) {
        mEditorUiGpuTimer->Shutdown();
    }
    if (mGamePostProcessRenderer) {
        mGamePostProcessRenderer->Shutdown();
    }
    if (mUGCPreviewPostProcessRenderer) {
        mUGCPreviewPostProcessRenderer->Shutdown();
    }
    mRenderTargets.Shutdown();
}

unsigned int GameFrameRenderer::GetUGCPreviewTexture() const
{
    return mRenderTargets.GetUGCPreviewTexture();
}

void GameFrameRenderer::DrawGameFrame(
    unsigned int destinationFramebuffer,
    int framebufferWidth,
    int framebufferHeight)
{
    mRenderer3D.RenderGameplayShadowMap();
    const bool isPostProcessActive =
        mGamePostProcessRenderer &&
        mGamePostProcessRenderer->BeginScene(
            framebufferWidth,
            framebufferHeight);
    if (!isPostProcessActive) {
        glBindFramebuffer(GL_FRAMEBUFFER, destinationFramebuffer);
    }
    glViewport(0, 0, framebufferWidth, framebufferHeight);
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mUIRenderer.DrawSkyBox();

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    mRenderer3D.Draw();

    if (isPostProcessActive) {
        mGamePostProcessRenderer->CompositeTo(
            destinationFramebuffer,
            framebufferWidth,
            framebufferHeight);
    }

    glDisable(GL_DEPTH_TEST);
    const auto gameUiStartTime = std::chrono::steady_clock::now();
    mGameUiGpuTimer->Begin();
    mUIRenderer.DrawGameContent();
    mGameUiGpuTimer->End();
    mPerformanceTracker.RecordGameUiCpuDuration(
        std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - gameUiStartTime).count());
    glEnable(GL_DEPTH_TEST);
}

void GameFrameRenderer::DrawUGCPreviewFrame(
    const GameFrameRenderState& renderState)
{
    const int previewWidth = mRenderTargets.GetUGCPreviewWidth();
    const int previewHeight = mRenderTargets.GetUGCPreviewHeight();
    if (previewWidth <= 0 || previewHeight <= 0) {
        return;
    }

    const bool isPostProcessActive =
        mUGCPreviewPostProcessRenderer &&
        mUGCPreviewPostProcessRenderer->BeginScene(
            previewWidth,
            previewHeight);
    if (!isPostProcessActive) {
        glBindFramebuffer(
            GL_FRAMEBUFFER,
            mRenderTargets.GetUGCPreviewFramebuffer());
    }
    glViewport(0, 0, previewWidth, previewHeight);
    glClearColor(0.025f, 0.035f, 0.075f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    mUIRenderer.DrawSkyBox(previewWidth, previewHeight);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    const CameraPose editorPose =
        mCameraSystem.GetDebugCameraPose();
    const float previewFocusY =
        mUGCPreviewController.UpdateFocusY(
            renderState.deltaTimeSeconds);
    glm::vec3 previewTarget = editorPose.target;
    previewTarget.y = previewFocusY;

    const glm::vec3 basePreviewDirection =
        glm::normalize(glm::vec3(1.0f, 0.75f, 1.0f));
    const float previewYawRadians =
        mUGCPreviewController.GetYaw();
    const float previewYawCosine = std::cos(previewYawRadians);
    const float previewYawSine = std::sin(previewYawRadians);
    const glm::vec3 previewDirection = glm::normalize(glm::vec3(
        basePreviewDirection.x * previewYawCosine +
            basePreviewDirection.z * previewYawSine,
        mUGCPreviewController.IsViewedFromBelow()
            ? -basePreviewDirection.y
            : basePreviewDirection.y,
        -basePreviewDirection.x * previewYawSine +
            basePreviewDirection.z * previewYawCosine));

    constexpr float previewFieldOfViewDegrees = 55.0f;
    const float editorViewDistance =
        renderState.isUGCOrthographicView
        ? mUGCPreviewController.GetOrthographicHalfHeight() /
              std::tan(glm::radians(previewFieldOfViewDegrees) * 0.5f)
        : glm::length(editorPose.position - editorPose.target);
    const float previewDistance = glm::clamp(
        editorViewDistance * 0.45f, 3.0f, 100.0f);
    const glm::vec3 previewPosition =
        previewTarget + previewDirection * previewDistance;
    const glm::vec3 previewUp =
        mUGCPreviewController.IsViewedFromBelow()
        ? glm::vec3(0.0f, -1.0f, 0.0f)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat4 view = glm::lookAt(
        previewPosition, previewTarget, previewUp);
    const glm::mat4 projection = glm::perspective(
        glm::radians(previewFieldOfViewDegrees),
        static_cast<float>(previewWidth) / previewHeight,
        0.1f,
        1000.0f);

    mRenderer3D.RenderShadowMap(previewTarget);

    mRenderer3D.DrawScene(
        view,
        projection,
        previewPosition,
        UGCSceneLayerRenderMode::HighlightEditingLayerWithoutDimming,
        mUGCPreviewController.GetEditLayer());

    if (isPostProcessActive) {
        mUGCPreviewPostProcessRenderer->CompositeTo(
            mRenderTargets.GetUGCPreviewFramebuffer(),
            previewWidth,
            previewHeight);
    }
}
