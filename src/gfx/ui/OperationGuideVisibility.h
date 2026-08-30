#pragma once

#include <string_view>

struct OperationGuideDisplayState {
    int currentStageNumber = 0;
    int currentPlanetNumber = 0;
    bool isUGCPlaytestActive = false;
};

bool ShouldShowOperationGuideElement(
    std::string_view elementId,
    const OperationGuideDisplayState& displayState);
