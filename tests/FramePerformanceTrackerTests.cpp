#include "TestSupport.h"

#include "gfx/performance/FramePerformanceTracker.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void ViewportDurationsTrackBothRenderedViewports()
{
    FramePerformanceTracker tracker;

    tracker.RecordViewportCpuDuration(0, 1.25f);
    tracker.RecordViewportCpuDuration(1, 2.5f);
    tracker.RecordViewportGpuDuration(0, 0.75f);
    tracker.RecordViewportGpuDuration(1, 1.5f);

    const FramePerformanceMetrics& metrics = tracker.GetMetrics();
    ExpectNear(
        1.25f, metrics.firstViewportRenderMilliseconds, 0.0001f,
        "first viewport CPU duration");
    ExpectNear(
        2.5f, metrics.secondViewportRenderMilliseconds, 0.0001f,
        "second viewport CPU duration");
    ExpectNear(
        0.75f, metrics.firstViewportGpuMilliseconds, 0.0001f,
        "first viewport GPU duration");
    ExpectNear(
        1.5f, metrics.secondViewportGpuMilliseconds, 0.0001f,
        "second viewport GPU duration");
    ExpectEqual(2, metrics.renderedViewportCount, "rendered viewport count");
    ExpectTrue(
        metrics.hasFirstViewportGpuMeasurement,
        "first viewport GPU measurement flag");
    ExpectTrue(
        metrics.hasSecondViewportGpuMeasurement,
        "second viewport GPU measurement flag");
}

void BeginFrameClearsPerFrameCpuValuesAndPreservesLatestGpuValues()
{
    FramePerformanceTracker tracker;
    tracker.RecordViewportCpuDuration(0, 3.0f);
    tracker.RecordViewportGpuDuration(0, 2.0f);
    tracker.RecordGameUpdateDuration(4.0f);
    tracker.RecordGameUiCpuDuration(5.0f);

    tracker.BeginFrame();

    const FramePerformanceMetrics& metrics = tracker.GetMetrics();
    ExpectNear(
        0.0f, metrics.firstViewportRenderMilliseconds, 0.0001f,
        "reset viewport CPU duration");
    ExpectNear(
        0.0f, metrics.gameUpdateMilliseconds, 0.0001f,
        "reset game update duration");
    ExpectNear(
        0.0f, metrics.gameUiCpuMilliseconds, 0.0001f,
        "reset game UI CPU duration");
    ExpectEqual(0, metrics.renderedViewportCount, "reset viewport count");
    ExpectNear(
        2.0f, metrics.firstViewportGpuMilliseconds, 0.0001f,
        "preserved latest GPU duration");
    ExpectTrue(
        metrics.hasFirstViewportGpuMeasurement,
        "preserved GPU measurement flag");
}

}

void RegisterFramePerformanceTrackerTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "FramePerformanceTracker.ViewportDurationsTrackBothRenderedViewports",
        ViewportDurationsTrackBothRenderedViewports);
    tests.emplace_back(
        "FramePerformanceTracker.BeginFrameClearsPerFrameCpuValuesAndPreservesLatestGpuValues",
        BeginFrameClearsPerFrameCpuValuesAndPreservesLatestGpuValues);
}
