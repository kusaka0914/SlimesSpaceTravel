#pragma once

#include "gfx/GameRenderTargets.h"

#include <memory>

struct GLFWwindow;
class CameraSystem;
class FramePerformanceTracker;
class GpuDurationTimer;
class PostProcessRenderer;
class Renderer3D;
class UGCPreviewController;
class UIRenderer;

struct GameFrameRenderState {
    bool isDebugEditorShowing = false;
    bool isUGCModeActive = false;
    bool isUGCWorkBrowserShowing = false;
    bool isUGCOrthographicView = false;
    float deltaTimeSeconds = 0.0f;
};

class GameFrameRenderer {
public:
    GameFrameRenderer(
        GLFWwindow& window,
        Renderer3D& renderer3D,
        UIRenderer& uiRenderer,
        CameraSystem& cameraSystem,
        UGCPreviewController& ugcPreviewController,
        FramePerformanceTracker& performanceTracker);
    ~GameFrameRenderer();

    void Render(const GameFrameRenderState& renderState);
    void PollGpuPerformanceMeasurements();
    void Shutdown();
    unsigned int GetUGCPreviewTexture() const;

private:
    void DrawGameFrame(
        unsigned int destinationFramebuffer,
        int framebufferWidth,
        int framebufferHeight);
    void DrawUGCPreviewFrame(const GameFrameRenderState& renderState);

    GLFWwindow& mWindow;
    Renderer3D& mRenderer3D;
    UIRenderer& mUIRenderer;
    CameraSystem& mCameraSystem;
    UGCPreviewController& mUGCPreviewController;
    FramePerformanceTracker& mPerformanceTracker;
    GameRenderTargets mRenderTargets;
    std::unique_ptr<PostProcessRenderer> mGamePostProcessRenderer;
    std::unique_ptr<PostProcessRenderer> mUGCPreviewPostProcessRenderer;
    std::unique_ptr<GpuDurationTimer> mGameUiGpuTimer;
    std::unique_ptr<GpuDurationTimer> mEditorUiGpuTimer;
};
