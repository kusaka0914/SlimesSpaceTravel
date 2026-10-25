#include "editor/ui/UICanvasEditHistory.h"

#include <algorithm>
#include <utility>

UICanvasEditHistory::UICanvasEditHistory(std::size_t maximumUndoCount)
    : mMaximumUndoCount(std::max<std::size_t>(1, maximumUndoCount))
{
}

void UICanvasEditHistory::Capture(const UILoadSystem& uiLoadSystem)
{
    UICanvasEditSnapshot snapshot;
    snapshot.customElements = uiLoadSystem.GetCustomElements();
    snapshot.textureInfos = uiLoadSystem.GetEditableTextureInfos();
    snapshot.textInfos = uiLoadSystem.GetEditableTextInfos();
    mUndoSnapshots.push_back(std::move(snapshot));

    if (mUndoSnapshots.size() > mMaximumUndoCount) {
        mUndoSnapshots.erase(mUndoSnapshots.begin());
    }
}

bool UICanvasEditHistory::RestoreLatest(UILoadSystem& uiLoadSystem, const std::function<bool()>& saveChanges)
{
    if (mUndoSnapshots.empty()) {
        return false;
    }

    UICanvasEditSnapshot candidate = mUndoSnapshots.back();
    auto swapDefinitions = [&]() {
        candidate.customElements.swap(uiLoadSystem.GetCustomElements());
        candidate.textureInfos.swap(uiLoadSystem.GetEditableTextureInfos());
        candidate.textInfos.swap(uiLoadSystem.GetEditableTextInfos());
    };
    swapDefinitions();
    try {
        if (!saveChanges()) {
            swapDefinitions();
            return false;
        }
    } catch (...) {
        swapDefinitions();
        throw;
    }
    mUndoSnapshots.pop_back();
    uiLoadSystem.ClearCustomVisibilityOverrides();
    return true;
}

void UICanvasEditHistory::DiscardLatest()
{
    if (!mUndoSnapshots.empty()) {
        mUndoSnapshots.pop_back();
    }
}
