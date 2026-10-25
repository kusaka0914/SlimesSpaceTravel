#pragma once

#include <string_view>

// 操作案内の段階的な解放と、1人用案内の表示判定に使う状態。
struct OperationGuideDisplayState {
    int currentStageNumber = 0;
    int currentPlanetNumber = 0;
    bool isUGCPlaytestActive = false;
    bool isTwoPlayerMode = false;
};

// ステージ進行・UGCプレイテスト・2人プレイの条件で表示可否を返す。対象外のIDは表示を許可する。
bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState);
