#pragma once

#include "system/actor_loader/PlatformStageConfig.h"

class Platform;

void ApplyPlatformStageConfig(
    Platform& platform,
    const PlatformStageConfig& config);
