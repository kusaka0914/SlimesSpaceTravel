#include "gfx/performance/FramePerformanceTracker.h"

#include <algorithm>

void FramePerformanceTracker::BeginFrame()
{
    mMetrics.gameUpdateMilliseconds = 0.0f;
    mMetrics.firstViewportRenderMilliseconds = 0.0f;
    mMetrics.secondViewportRenderMilliseconds = 0.0f;
    mMetrics.gameUiCpuMilliseconds = 0.0f;
    mMetrics.editorUiCpuMilliseconds = 0.0f;
    mMetrics.presentationWaitMilliseconds = 0.0f;
    mMetrics.renderedViewportCount = 0;
}

void FramePerformanceTracker::RecordViewportCpuDuration(
    int viewportIndex,
    float durationMilliseconds)
{
    if (viewportIndex == 0) {
        mMetrics.firstViewportRenderMilliseconds = durationMilliseconds;
    } else if (viewportIndex == 1) {
        mMetrics.secondViewportRenderMilliseconds = durationMilliseconds;
    } else {
        return;
    }

    mMetrics.renderedViewportCount = std::max(
        mMetrics.renderedViewportCount,
        viewportIndex + 1);
}

void FramePerformanceTracker::RecordViewportGpuDuration(
    int viewportIndex,
    float durationMilliseconds)
{
    if (viewportIndex == 0) {
        mMetrics.firstViewportGpuMilliseconds = durationMilliseconds;
        mMetrics.hasFirstViewportGpuMeasurement = true;
    } else if (viewportIndex == 1) {
        mMetrics.secondViewportGpuMilliseconds = durationMilliseconds;
        mMetrics.hasSecondViewportGpuMeasurement = true;
    }
}

void FramePerformanceTracker::RecordTotalDuration(
    float durationMilliseconds)
{
    mMetrics.totalMilliseconds = durationMilliseconds;
}

void FramePerformanceTracker::RecordGameUpdateDuration(
    float durationMilliseconds)
{
    mMetrics.gameUpdateMilliseconds = durationMilliseconds;
}

void FramePerformanceTracker::RecordGameUiCpuDuration(
    float durationMilliseconds)
{
    mMetrics.gameUiCpuMilliseconds = durationMilliseconds;
}

void FramePerformanceTracker::RecordEditorUiCpuDuration(
    float durationMilliseconds)
{
    mMetrics.editorUiCpuMilliseconds = durationMilliseconds;
}

void FramePerformanceTracker::RecordGameUiGpuDuration(
    float durationMilliseconds)
{
    mMetrics.gameUiGpuMilliseconds = durationMilliseconds;
    mMetrics.hasGameUiGpuMeasurement = true;
}

void FramePerformanceTracker::RecordEditorUiGpuDuration(
    float durationMilliseconds)
{
    mMetrics.editorUiGpuMilliseconds = durationMilliseconds;
    mMetrics.hasEditorUiGpuMeasurement = true;
}

void FramePerformanceTracker::RecordPresentationWaitDuration(
    float durationMilliseconds)
{
    mMetrics.presentationWaitMilliseconds = durationMilliseconds;
}

const FramePerformanceMetrics& FramePerformanceTracker::GetMetrics() const
{
    return mMetrics;
}
