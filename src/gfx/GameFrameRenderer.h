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

// そのフレームの表示モード。deltaTimeSecondsはUGCプレビューの注目位置補間に使う経過秒数。
struct GameFrameRenderState {
    bool isDebugEditorShowing = false;
    bool isUGCModeActive = false;
    bool isUGCWorkBrowserShowing = false;
    bool isUGCOrthographicView = false;
    float deltaTimeSeconds = 0.0f;
};

// 影・3D・ポストプロセス・UI・画面提示の順序を統括し、エディター用描画先とGPU計測を所有する。
// コンストラクタで受け取るウィンドウ・各描画／ゲーム機能への参照は非所有。
class GameFrameRenderer {
public:
    GameFrameRenderer(GLFWwindow& window, Renderer3D& renderer3D, UIRenderer& uiRenderer, CameraSystem& cameraSystem,
                      UGCPreviewController& ugcPreviewController, FramePerformanceTracker& performanceTracker);
    ~GameFrameRenderer();

    // ゲーム画面と必要な編集画面を描画してバッファを交換し、CPU所要時間を記録する。
    void Render(const GameFrameRenderState& renderState);

    // 完了済みのGPU計測結果だけを取得し、PerformanceTrackerへ反映する。
    void PollGpuPerformanceMeasurements();

    // OpenGLコンテキストが有効な間に呼び、所有する描画先・ポストプロセス・GPU計測を解放する。
    void Shutdown();
    unsigned int GetUGCPreviewTexture() const;

private:
    void DrawGameFrame(unsigned int destinationFramebuffer, int framebufferWidth, int framebufferHeight);
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
