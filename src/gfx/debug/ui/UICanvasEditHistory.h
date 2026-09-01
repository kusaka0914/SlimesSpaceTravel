#pragma once

#include "system/UILoadSystem.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

struct UICanvasEditSnapshot {
    std::vector<UILoadSystem::CustomElement> customElements;
    std::unordered_map<std::string, UILoadSystem::TextureInfo> textureInfos;
    std::unordered_map<std::string, UILoadSystem::TextInfo> textInfos;
};

class UICanvasEditHistory {
public:
    explicit UICanvasEditHistory(std::size_t maximumUndoCount = 20);

    void Capture(const UILoadSystem& uiLoadSystem);
    bool RestoreLatest(UILoadSystem& uiLoadSystem);
    void DiscardLatest();

private:
    std::size_t mMaximumUndoCount;
    std::vector<UICanvasEditSnapshot> mUndoSnapshots;
};
