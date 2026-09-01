#include "gfx/debug/ui/UICanvasEditHistory.h"

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

bool UICanvasEditHistory::RestoreLatest(UILoadSystem& uiLoadSystem)
{
    if (mUndoSnapshots.empty()) {
        return false;
    }

    UICanvasEditSnapshot snapshot = std::move(mUndoSnapshots.back());
    mUndoSnapshots.pop_back();
    uiLoadSystem.GetCustomElements() = std::move(snapshot.customElements);
    uiLoadSystem.GetEditableTextureInfos() = std::move(snapshot.textureInfos);
    uiLoadSystem.GetEditableTextInfos() = std::move(snapshot.textInfos);
    uiLoadSystem.ClearCustomVisibilityOverrides();
    return true;
}

void UICanvasEditHistory::DiscardLatest()
{
    if (!mUndoSnapshots.empty()) {
        mUndoSnapshots.pop_back();
    }
}
