#include <GL/glew.h>

#include "RuntimeFrameLoopTests.h"

#include "app/Game.h"
#include "support/TestFileSupport.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

void RecordMetric(const char* name, std::vector<float> samples)
{
    if (samples.empty()) {
        return;
    }
    std::sort(samples.begin(), samples.end());
    const auto percentile = [&](double fraction) {
        return samples[static_cast<std::size_t>(std::ceil(fraction * samples.size())) - 1];
    };
    std::cout << "FRAME_LOOP_METRIC {\"name\":" << std::quoted(name) << ",\"samples\":" << samples.size()
              << ",\"median_ms\":" << percentile(0.5) << ",\"p95_ms\":" << percentile(0.95)
              << ",\"max_ms\":" << samples.back() << "}\n";
}

void JoinSecondPlayer(Game& game)
{
    game.TryCreatePlayer2();
    ExpectTrue(game.GetIsPlayer2Joined(), "frame loop has second player");
}

} // namespace

void MeasureRuntimeFrameLoop(Game& game, int frameCount, const std::string& scenario)
{
    ExpectTrue(scenario == "one-player" || scenario == "two-player" || scenario == "ugc" || scenario == "cycle",
               "known frame loop scenario");
    game.SetDebugEditorShowing(false);
    if (scenario == "two-player") {
        JoinSecondPlayer(game);
    } else if (scenario == "ugc") {
        ExpectTrue(game.StartUGCMode(), "frame loop starts UGC");
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(game.GetWindow(), &width, &height);
    std::cout << "FRAME_LOOP_CONTEXT {\"scenario\":" << std::quoted(scenario)
              << ",\"renderer\":" << std::quoted(reinterpret_cast<const char*>(glGetString(GL_RENDERER)))
              << ",\"gl_version\":" << std::quoted(reinterpret_cast<const char*>(glGetString(GL_VERSION)))
              << ",\"framebuffer_width\":" << width << ",\"framebuffer_height\":" << height << "}\n";

    std::vector<float> frameIntervals;
    std::vector<float> workDurations;
    std::vector<float> updateDurations;
    std::vector<float> presentationDurations;
    std::vector<float> reloadDurations;
    const int warmupFrames = std::min(60, frameCount / 4);
    int completed = 0;
    int twoPlayerFrames = 0;
    auto previousFrame = Clock::now();
    game.RunLoop([&](const FramePerformanceMetrics& metrics) {
        const auto now = Clock::now();
        ++completed;
        ExpectTrue(std::isfinite(metrics.totalMilliseconds) && metrics.totalMilliseconds >= 0, "finite frame duration");
        ExpectEqual(static_cast<GLuint>(GL_NO_ERROR), glGetError(), "frame loop leaves no GL error");
        if (metrics.renderedViewportCount == 2) {
            ++twoPlayerFrames;
        }
        if (completed > warmupFrames) {
            // totalはソフトウェアのFPS待機を含まない。実フレーム間隔は別に記録する。
            frameIntervals.push_back(std::chrono::duration<float, std::milli>(now - previousFrame).count());
            workDurations.push_back(metrics.totalMilliseconds);
            updateDurations.push_back(metrics.gameUpdateMilliseconds);
            presentationDurations.push_back(metrics.presentationWaitMilliseconds);
        }
        previousFrame = now;

        if (scenario == "cycle") {
            switch (completed % 240) {
            case 30:
                JoinSecondPlayer(game);
                break;
            case 60:
                ExpectTrue(game.StartUGCMode(), "cycle starts UGC");
                break;
            case 120:
                game.ExitUGCMode();
                break;
            case 0: {
                // ExitUGCModeはフェード後に完了するため、通常ループで更新した後に確認する。
                ExpectFalse(game.GetIsUGCMode(), "cycle exits UGC after fade");
                game.SetDebugEditorShowing(false);
                const auto reloadStart = Clock::now();
                ExpectTrue(game.ReloadCurrentStage(), "cycle reloads stage");
                reloadDurations.push_back(std::chrono::duration<float, std::milli>(Clock::now() - reloadStart).count());
                break;
            }
            default:
                break;
            }
        }
        if (completed == frameCount) {
            glfwSetWindowShouldClose(game.GetWindow(), GLFW_TRUE);
        }
    });
    ExpectEqual(frameCount, completed, "frame loop completes requested frames");
    if (scenario == "two-player" || (scenario == "cycle" && frameCount >= 60)) {
        ExpectTrue(twoPlayerFrames > 0, "frame loop renders both players");
    }
    RecordMetric("frame_interval", std::move(frameIntervals));
    RecordMetric("frame_work_including_presentation", std::move(workDurations));
    RecordMetric("update", std::move(updateDurations));
    RecordMetric("presentation_wait", std::move(presentationDurations));
    RecordMetric("synchronous_stage_reload", std::move(reloadDurations));
    // GPU計測は非同期の最新値であり、重複する値を独立サンプルとして集計しない。
    const auto& metrics = game.GetFramePerformanceMetrics();
    std::cout << "FRAME_LOOP_RESULT {\"completed_frames\":" << completed << ",\"warmup_frames\":" << warmupFrames
              << ",\"two_player_frames\":" << twoPlayerFrames
              << ",\"has_latest_viewport_gpu\":" << (metrics.hasFirstViewportGpuMeasurement ? "true" : "false")
              << ",\"latest_viewport_gpu_ms\":" << metrics.firstViewportGpuMilliseconds << "}\n";
}
