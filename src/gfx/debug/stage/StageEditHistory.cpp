#include "gfx/debug/stage/StageEditHistory.h"

#include <algorithm>

StageEditHistory::StageEditHistory(std::size_t maximumUndoCount)
    : mMaximumUndoCount(std::max<std::size_t>(1, maximumUndoCount))
{
}

void StageEditHistory::BindToStage(const std::string& stagePath)
{
    if (mStagePath != stagePath) {
        Clear();
        mStagePath = stagePath;
    }
}

void StageEditHistory::PushUndoSnapshot(const std::string& yamlText)
{
    mLastGroupedYamlText.clear();
    mUndoSnapshots.push_back(yamlText);
    mRedoSnapshots.clear();

    if (mUndoSnapshots.size() > mMaximumUndoCount) {
        mUndoSnapshots.erase(mUndoSnapshots.begin());
    }
}

void StageEditHistory::RecordSavedChange(const std::string& before, const std::string& after,
                                         StageEditGrouping grouping)
{
    if (before == after) {
        return;
    }
    if (grouping != StageEditGrouping::ContinueOperation || mLastGroupedYamlText != before) {
        PushUndoSnapshot(before);
    }
    mRedoSnapshots.clear();
    mLastGroupedYamlText = after;
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
    mLastGroupedYamlText.clear();
    if (mUndoSnapshots.empty()) {
        return;
    }
    mUndoSnapshots.pop_back();
    mRedoSnapshots.push_back(currentYamlText);
}

void StageEditHistory::CommitRedo(const std::string& currentYamlText)
{
    mLastGroupedYamlText.clear();
    if (mRedoSnapshots.empty()) {
        return;
    }
    mRedoSnapshots.pop_back();
    mUndoSnapshots.push_back(currentYamlText);
}

void StageEditHistory::Clear()
{
    mLastGroupedYamlText.clear();
    mUndoSnapshots.clear();
    mRedoSnapshots.clear();
}

std::size_t StageEditHistory::GetUndoCount() const
{
    return mUndoSnapshots.size();
}

std::size_t StageEditHistory::GetRedoCount() const
{
    return mRedoSnapshots.size();
}
