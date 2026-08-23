#pragma once

#include <glm/glm.hpp>

class Actor;
class Planet;
class Player;
class PlayerMovement;
class Stage;

class PlayerPlanetGravityController {
public:
    void Update(Player& player, PlayerMovement& movement, float deltaTime);

    void OnJumpStarted(
        Player& player,
        PlayerMovement& movement);

    void RestartFallbackDelayForAirborneAction(
        const Player& player);

    void OnGroundRayCastSucceeded();

    void OnLanded(Player& player, PlayerMovement& movement);
    void OnRespawned();

    bool IsJumpGravityActive() const { return mIsJumpSwitchingActive; }
    bool IsEllipseAirborneGravityActive(const Player& player) const;
    bool ShouldUseEllipseSurfaceGravity(const Player& player) const;
    glm::vec3 CalculateAirbornePhysicsUpDirection(
        const Player& player) const;
    bool ShouldAcceptLandingSurface(
        const glm::vec3& surfaceNormal) const;
    bool WasFallbackAppliedThisJump() const { return mFallbackAppliedThisJump; }

private:
    void SwitchToPlanet(Player& player, PlayerMovement& movement, Planet* nextPlanet);

    void ApplyCurrentPlanet(Player& player, PlayerMovement& movement, Planet* planet) const;

    bool TryActivateOverheadGravityRay(
        Player& player,
        PlayerMovement& movement,
        float deltaTime);

    void UpdateEllipseAirborneGravity(
        Player& player,
        bool isDodging,
        float deltaTime);
    bool ShouldActivateEllipseSurfaceGravity(
        const Player& player,
        const Planet& planet) const;
    void ApplyEllipseSurfaceAttraction(
        Player& player,
        const Planet& planet,
        float deltaTime) const;
    Planet* SelectNearbyAttractingPlanet(
        Player& player,
        PlayerMovement& movement);
    void ApplyNearbySurfaceAttraction(
        Player& player,
        const Planet& planet,
        float deltaTime) const;
    void SmoothAirborneUpVec(
        Player& player,
        const glm::vec3& targetUp,
        float deltaTime);

    Planet* ResolvePlanetFromGroundActor(Actor* groundActor) const;
    Planet* ResolveFallbackPlanet(const Player& player) const;

    int FindPlanetIndex(const Stage& stage, const Planet* planet) const;

private:
    // 惑星間の境界付近で判定が細かく反転するのを防ぐ。
    // 接地Actorから惑星を特定できなかった場合の補助検索距離。
    // 新しい惑星方向へ向きを変える速さ。
    // 2.0f: ゆっくり
    // 3.0f: ふわっと自然
    // 5.0f: 比較的速い
    static constexpr float gravityTurnSpeed = 12.0f;
    static constexpr float airborneMaximumTurnDegreesPerSecond = 540.0f;

    // 近接重力は、当たり判定に届く前から姿勢と落下方向を切り替える。
    // この距離外では従来どおり現在の惑星を維持する。
    static constexpr float nearbyAttractionRange = 3.0f;
    static constexpr float planetSwitchDistanceHysteresis = 0.15f;
    static constexpr float nearbyAttractionAcceleration = 42.0f;
    static constexpr float nearbyAttractionMaximumAcceleration = 70.0f;

    static constexpr float ellipseAttractionStartSurfaceDistance = 1.25f;
    static constexpr float ellipseDetachedTakeoffSurfaceDistance = 0.75f;
    static constexpr float ellipseAttractionPerDistance = 10.0f;
    static constexpr float ellipseNormalTransitionAcceleration = 32.0f;
    static constexpr float ellipseMaximumAttractionAcceleration = 50.0f;

    static constexpr float fallbackDelay = 2.0f;

    bool mIsJumpSwitchingActive = false;
    bool mIsOverheadGravityRayActive = false;
    bool mGroundRayHitThisFrame = false;
    bool mFallbackAppliedThisJump = false;
    bool mFallbackGravityActive = false;
    float mNoGroundRayDuration = 0.0f;
    float mEllipseJumpStartSurfaceDistance = 0.0f;
    bool mUseEllipseSurfaceGravity = false;
    bool mIsNearbySurfaceAttractionActive = false;
    Planet* mLastLandedPlanet = nullptr;
    glm::vec3 mOverheadGravityUpDirection{0.0f, 1.0f, 0.0f};

    // Actor::UpdateUpVecによる毎フレームの書き換えに影響されないよう、
    // 補間中の上方向をこのクラスで保持する。
    bool mSmoothedUpInitialized = false;
    glm::vec3 mSmoothedUpVec{0.0f, 1.0f, 0.0f};

};
