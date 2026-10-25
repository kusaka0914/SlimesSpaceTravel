#pragma once

#include "actor/player/PlayerTypes.h"

#include <functional>

class AudioSystem;
class ParticleSystem;
class PhysicsSystem;

// 戦闘が借用するサービスとアプリへの通知先。サービスと通知先はPlayerCombatより長く保持する。
// 操作方式は変更可能なため、攻撃開始時の値をreadControlStyleで取得する。
struct PlayerCombatDependencies {
    PhysicsSystem& physics;
    AudioSystem& audio;
    ParticleSystem* particles;
    std::function<PlayerControlStyle()> readControlStyle;
    std::function<void(int playerNum)> onAttackHit;
    std::function<void(int playerNum, int lowFrequency, int highFrequency, int durationMilliseconds)> vibrate;
};
