#include "gfx/debug/stage/StageEditHistory.h"

#include <algorithm>

StageEditHistory::StageEditHistory(std::size_t maximumUndoCount)
    : mMaximumUndoCount(std::max<std::size_t>(1, maximumUndoCount))
{
}

void StageEditHistory::PushUndoSnapshot(const std::string& yamlText)
{
    mUndoSnapshots.push_back(yamlText);
    mRedoSnapshots.clear();

    if (mUndoSnapshots.size() > mMaximumUndoCount) {
        mUndoSnapshots.erase(mUndoSnapshots.begin());
    }
}

const std::string* StageEditHistory::FindUndoSnapshot() const
{
    return mUndoSnapshots.empty() ? nullptr : &mUndoSnapshots.back();
}

const std::string* StageEditHistory::FindRedoSnapshot() const
{
    return mRedoSnapshots.empty() ? nullptr : &mRedoSnapshots.back();
}

void StageEditHistory::CommitUndo(const std::string& currentYamlText)
{
    if (mUndoSnapshots.empty()) {
        return;
    }
    mUndoSnapshots.pop_back();
    mRedoSnapshots.push_back(currentYamlText);
}

void StageEditHistory::CommitRedo(const std::string& currentYamlText)
{
    if (mRedoSnapshots.empty()) {
        return;
    }
    mRedoSnapshots.pop_back();
    mUndoSnapshots.push_back(currentYamlText);
}

std::size_t StageEditHistory::GetUndoCount() const
{
    return mUndoSnapshots.size();
}

std::size_t StageEditHistory::GetRedoCount() const
{
    return mRedoSnapshots.size();
}
