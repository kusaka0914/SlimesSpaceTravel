#pragma once

#include "actor/player/PlanetGravityCandidateSelector.h"

#include <glm/glm.hpp>

class Actor;
class Planet;
class Player;
class PlayerMovement;
class Stage;

class PlayerPlanetGravityController {
public:
    void Update(Player& player, PlayerMovement& movement, float deltaTime);

    void OnJumpStarted();

    void OnGroundRayCastSucceeded();

    void OnLanded(Player& player, PlayerMovement& movement);
    void OnRespawned();

    bool IsJumpGravityActive() const { return mIsJumpSwitchingActive; }
    bool WasFallbackAppliedThisJump() const { return mFallbackAppliedThisJump; }

private:
    bool ShouldSwitchPlanet(const Player& player, const PlanetDistanceCandidate& candidate) const;

    void SwitchToPlanet(Player& player, PlayerMovement& movement, Planet* nextPlanet);

    void ApplyCurrentPlanet(Player& player, PlayerMovement& movement, Planet* planet) const;

    void SmoothAirborneUpVec(Player& player, float deltaTime);

    Planet* ResolvePlanetFromGroundActor(Actor* groundActor) const;

    int FindPlanetIndex(const Stage& stage, const Planet* planet) const;

private:
    // 惑星間の境界付近で判定が細かく反転するのを防ぐ。
    static constexpr float switchDistanceMargin = 0.3f;

    // 接地Actorから惑星を特定できなかった場合の補助検索距離。
    static constexpr float landedPlanetSearchDistance = 2.0f;

    // 新しい惑星方向へ向きを変える速さ。
    // 2.0f: ゆっくり
    // 3.0f: ふわっと自然
    // 5.0f: 比較的速い
    static constexpr float gravityTurnSpeed = 4.0f;

    static constexpr float fallbackDelay = 2.0f;

    bool mIsJumpSwitchingActive = false;
    bool mGroundRayHitThisFrame = false;
    bool mFallbackAppliedThisJump = false;
    bool mFallbackGravityActive = false;
    float mNoGroundRayDuration = 0.0f;

    // Actor::UpdateUpVecによる毎フレームの書き換えに影響されないよう、
    // 補間中の上方向をこのクラスで保持する。
    bool mSmoothedUpInitialized = false;
    glm::vec3 mSmoothedUpVec{0.0f, 1.0f, 0.0f};

    PlanetGravityCandidateSelector mCandidateSelector;
};
