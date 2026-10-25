#pragma once

#include "system/actor_loader/PlatformStageConfig.h"

class Platform;

// 設定された振る舞いのComponentをPlatformへ追加・設定する。設定のないComponentは削除しない。
void ApplyPlatformStageConfig(Platform& platform, const PlatformStageConfig& config);
