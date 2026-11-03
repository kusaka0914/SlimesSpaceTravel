#pragma once

#include <functional>

class AudioSystem;
class ParticleSystem;
class PhysicsSystem;

// 戦闘が借用するサービスとアプリへの通知先。サービスと通知先はPlayerCombatより長く保持する。
struct PlayerCombatDependencies {
    PhysicsSystem& physics;
    AudioSystem& audio;
    ParticleSystem* particles;
    std::function<void(int playerNum)> onAttackHit;
    std::function<void(int playerNum, int lowFrequency, int highFrequency, int durationMilliseconds)> vibrate;
    std::function<void(float seconds)> requestHitStop{};
    std::function<void()> onComboFinisherHit{};
};
