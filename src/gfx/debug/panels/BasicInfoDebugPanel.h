#pragma once

class PerformanceDebugPanel;

class BasicInfoDebugPanel {
public:
    BasicInfoDebugPanel(
        PerformanceDebugPanel& performancePanel);

    void Draw();

private:
    PerformanceDebugPanel& mPerformancePanel;
};
