#pragma once

#include <algorithm>

namespace DebugEditorLayout {
constexpr float StandardTopBarHeight = 72.0f;
constexpr float StageTopBarHeight = 112.0f;
constexpr float GameViewportToolbarHeight = 38.0f;

inline float CalculateTopBarHeight(bool showsStageTools)
{
    return showsStageTools
        ? StageTopBarHeight
        : StandardTopBarHeight;
}

inline float CalculateHierarchyWidth(float workspaceWidth)
{
    return std::clamp(workspaceWidth * 0.18f, 250.0f, 340.0f);
}

inline float CalculateToolPanelWidth(float workspaceWidth)
{
    return std::clamp(workspaceWidth * 0.34f, 440.0f, 760.0f);
}

inline float CalculateAssetBrowserHeight(float workspaceHeight)
{
    return std::clamp(workspaceHeight * 0.25f, 210.0f, 300.0f);
}
}
