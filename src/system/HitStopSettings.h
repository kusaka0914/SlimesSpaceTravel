#pragma once

#include <algorithm>
#include <cmath>

// 戦闘イベントによる停止時間（秒）。地上・空中共通コンボの通常命中は各段の設定を使う。
struct HitStopSettings {
    float guardBreakSeconds = 0.1f;
    float airStrongAttackSeconds = 0.4f;
    float airSlamSeconds = 0.4f;
    float counterSeconds = 0.3f;

    void Sanitize()
    {
        const auto clamp = [](float value, float fallback) {
            return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : fallback;
        };
        guardBreakSeconds = clamp(guardBreakSeconds, 0.1f);
        airStrongAttackSeconds = clamp(airStrongAttackSeconds, 0.4f);
        airSlamSeconds = clamp(airSlamSeconds, 0.4f);
        counterSeconds = clamp(counterSeconds, 0.3f);
    }
};
