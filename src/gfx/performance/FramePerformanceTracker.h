#pragma once

// 各時間の単位はミリ秒。GPU時間は対応するhasフラグがtrueの場合に有効。
struct FramePerformanceMetrics {
    float totalMilliseconds = 0.0f;
    float gameUpdateMilliseconds = 0.0f;
    float firstViewportRenderMilliseconds = 0.0f;
    float secondViewportRenderMilliseconds = 0.0f;
    float firstViewportGpuMilliseconds = 0.0f;
    float secondViewportGpuMilliseconds = 0.0f;
    float gameUiCpuMilliseconds = 0.0f;
    float gameUiGpuMilliseconds = 0.0f;
    float editorUiCpuMilliseconds = 0.0f;
    float editorUiGpuMilliseconds = 0.0f;
    float presentationWaitMilliseconds = 0.0f;
    int renderedViewportCount = 0;
    bool hasFirstViewportGpuMeasurement = false;
    bool hasSecondViewportGpuMeasurement = false;
    bool hasGameUiGpuMeasurement = false;
    bool hasEditorUiGpuMeasurement = false;
};

// フレーム内のCPU時間と、遅れて取得されるGPU時間の最新値を保持する。
class FramePerformanceTracker {
public:
    // フレーム内で集計するCPU時間と描画数をリセットする。totalとGPU計測値は次の記録まで保持する。
    void BeginFrame();

    // viewportIndexは0または1。それ以外は無視する。CPU時間の記録で描画数も更新する。
    void RecordViewportCpuDuration(int viewportIndex, float durationMilliseconds);
    void RecordViewportGpuDuration(int viewportIndex, float durationMilliseconds);

    void RecordTotalDuration(float durationMilliseconds);
    void RecordGameUpdateDuration(float durationMilliseconds);
    void RecordGameUiCpuDuration(float durationMilliseconds);
    void RecordEditorUiCpuDuration(float durationMilliseconds);
    void RecordGameUiGpuDuration(float durationMilliseconds);
    void RecordEditorUiGpuDuration(float durationMilliseconds);
    void RecordPresentationWaitDuration(float durationMilliseconds);

    const FramePerformanceMetrics& GetMetrics() const;

private:
    FramePerformanceMetrics mMetrics;
};
