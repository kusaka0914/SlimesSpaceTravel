#pragma once

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

class FramePerformanceTracker {
public:
    void BeginFrame();
    void RecordViewportCpuDuration(
        int viewportIndex,
        float durationMilliseconds);
    void RecordViewportGpuDuration(
        int viewportIndex,
        float durationMilliseconds);
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
