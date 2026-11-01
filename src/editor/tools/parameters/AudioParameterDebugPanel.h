#pragma once

#include "editor/core/DebugPanel.h"

#include <string>

// 音量を実行時に調整し、音源の試聴と音量設定の保存・再読込を提供する。
class AudioParameterDebugPanel : public DebugPanel {
public:
    explicit AudioParameterDebugPanel(DebugEditorContext& context);
    void Draw() override;

private:
    std::string mStatus;
    char mSearch[128]{};
};
