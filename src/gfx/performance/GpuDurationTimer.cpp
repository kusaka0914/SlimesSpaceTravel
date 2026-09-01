#include "gfx/performance/GpuDurationTimer.h"

GpuDurationTimer::~GpuDurationTimer()
{
    Shutdown();
}

void GpuDurationTimer::Begin()
{
    if (mActiveQuery != 0 || !GLEW_ARB_timer_query) {
        return;
    }

    Initialize();
    for (int attempt = 0; attempt < QuerySlotCount; ++attempt) {
        const int slotIndex =
            (mNextQuerySlot + attempt) % QuerySlotCount;
        if (mIsQueryPending[slotIndex]) {
            continue;
        }

        mActiveQuery = mQueries[slotIndex];
        mIsQueryPending[slotIndex] = true;
        mNextQuerySlot = (slotIndex + 1) % QuerySlotCount;
        glBeginQuery(GL_TIME_ELAPSED, mActiveQuery);
        return;
    }
}

void GpuDurationTimer::End()
{
    if (mActiveQuery == 0) {
        return;
    }

    glEndQuery(GL_TIME_ELAPSED);
    mActiveQuery = 0;
}

std::optional<float> GpuDurationTimer::PollCompletedMilliseconds()
{
    if (!mAreQueriesInitialized) {
        return std::nullopt;
    }

    for (int slotIndex = 0; slotIndex < QuerySlotCount; ++slotIndex) {
        if (!mIsQueryPending[slotIndex]) {
            continue;
        }

        GLint isResultAvailable = GL_FALSE;
        glGetQueryObjectiv(
            mQueries[slotIndex],
            GL_QUERY_RESULT_AVAILABLE,
            &isResultAvailable);
        if (isResultAvailable != GL_TRUE) {
            continue;
        }

        GLuint64 elapsedNanoseconds = 0;
        glGetQueryObjectui64v(
            mQueries[slotIndex],
            GL_QUERY_RESULT,
            &elapsedNanoseconds);
        mIsQueryPending[slotIndex] = false;
        return static_cast<float>(elapsedNanoseconds) / 1000000.0f;
    }

    return std::nullopt;
}

void GpuDurationTimer::Shutdown()
{
    if (!mAreQueriesInitialized) {
        return;
    }

    if (mActiveQuery != 0) {
        glEndQuery(GL_TIME_ELAPSED);
        mActiveQuery = 0;
    }

    glDeleteQueries(
        static_cast<GLsizei>(mQueries.size()),
        mQueries.data());
    mQueries = {};
    mIsQueryPending = {};
    mNextQuerySlot = 0;
    mAreQueriesInitialized = false;
}

void GpuDurationTimer::Initialize()
{
    if (mAreQueriesInitialized) {
        return;
    }

    glGenQueries(
        static_cast<GLsizei>(mQueries.size()),
        mQueries.data());
    mAreQueriesInitialized = true;
}
