#pragma once

class PerformanceDebugPanel;

// 基本情報のメニューを描画し、選択内容をPerformanceDebugPanelへ委譲する。参照先は所有しない。
class BasicInfoDebugPanel {
public:
    BasicInfoDebugPanel(PerformanceDebugPanel& performancePanel);

    void Draw();

private:
    PerformanceDebugPanel& mPerformancePanel;
};
