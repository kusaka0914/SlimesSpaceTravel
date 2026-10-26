#pragma once

#include <string_view>

// 操作案内の段階的な解放と、1人用案内の表示判定に使う状態。
struct OperationGuideDisplayState {
    int currentStageNumber = 0;
    int currentPlanetNumber = 0;
    bool isUGCPlaytestActive = false;
    bool isTwoPlayerMode = false;
    bool isPauseMenuOpen = false;
};

// operation画面の要素を判定する。ポーズ中は全解放し、通常ステージ2以降ではメニュー以外の常設案内を隠す。
bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState);
