#pragma once

#include <string_view>

// 操作案内の表示モードと、1人用案内の表示判定に使う状態。
struct OperationGuideDisplayState {
    int currentStageNumber = 0;
    int currentPlanetNumber = 0;
    bool isUGCPlaytestActive = false;
    bool isTwoPlayerMode = false;
    bool isPauseMenuOpen = false;
};

// operation画面の要素を判定する。通常プレイの常設操作案内は非表示とし、メニューは常時表示する。
// 通常プレイのポーズ中は全解放する。UGC試遊・作品プレイ中も常設操作案内は表示しない。
bool ShouldShowOperationGuideElement(std::string_view elementId, const OperationGuideDisplayState& displayState);
