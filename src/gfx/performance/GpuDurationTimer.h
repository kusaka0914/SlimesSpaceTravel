#pragma once

#include <GL/glew.h>

#include <array>
#include <optional>

class GpuDurationTimer {
public:
    ~GpuDurationTimer();

    void Begin();
    void End();
    std::optional<float> PollCompletedMilliseconds();
    void Shutdown();

private:
    static constexpr int QuerySlotCount = 3;

    void Initialize();

private:
    std::array<GLuint, QuerySlotCount> mQueries{};
    std::array<bool, QuerySlotCount> mIsQueryPending{};
    int mNextQuerySlot = 0;
    GLuint mActiveQuery = 0;
    bool mAreQueriesInitialized = false;
};
