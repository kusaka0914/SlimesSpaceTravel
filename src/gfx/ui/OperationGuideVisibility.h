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

// operation画面の要素を判定する。通常プレイの常設案内はステージ1だけ表示し、メニューは常時表示する。
// ポーズ中は全解放する。UGC試遊には通常ステージの表示制限を適用しない。
bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState);
