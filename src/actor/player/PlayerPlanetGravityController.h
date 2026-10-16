#pragma once

#include <glm/glm.hpp>

class Actor;
class Planet;
class Player;
class PlayerMovement;
class Stage;

// 離陸後の所属惑星と重力方向を管理する。頭上レイ・近接吸引・接地レイ失敗時の補正も担当する。
class PlayerPlanetGravityController {
public:
    // deltaTimeは秒。所属惑星・上方向・吸引速度を更新する。頭上レイによって接地を解除する場合がある。
    void Update(Player& player, PlayerMovement& movement, float deltaTime);

    // 離陸時に重力切替を開始する。tookOffFromPlatformなら上昇中は離陸時の重力方向を保持する。
    void OnJumpStarted(
        Player& player,
        PlayerMovement& movement,
        bool tookOffFromPlatform);

    // 空中行動に合わせて接地レイ未検出の待ち時間を戻す。接地中・切替無効時は何もしない。
    void RestartFallbackDelayForAirborneAction(
        const Player& player);

    void OnGroundRayCastSucceeded();

    // 空中の重力状態を解除する。接地ActorがPlanetなら所属と最終接地惑星も更新する。
    void OnLanded(Player& player, PlayerMovement& movement);
    void OnRespawned();

    bool IsJumpGravityActive() const { return mIsJumpSwitchingActive; }
    bool IsEllipseAirborneGravityActive(const Player& player) const;
    bool ShouldUseEllipseSurfaceGravity(const Player& player) const;

    // 物理計算用の単位上方向。描画の補間方向とは異なる場合があり、有効な方向なしではY上向きを返す。
    glm::vec3 CalculateAirbornePhysicsUpDirection(
        const Player& player) const;
    bool ShouldAcceptLandingSurface(
        const glm::vec3& surfaceNormal) const;
    bool WasFallbackAppliedThisJump() const { return mFallbackAppliedThisJump; }
    bool ShouldPreservePlatformTakeoffDirection() const
    {
        return mShouldPreservePlatformTakeoffDirection;
    }

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
    static constexpr float gravityTurnSpeed = 12.0f;
    static constexpr float airborneMaximumTurnDegreesPerSecond = 540.0f;

    // 衝突前から重力方向を切り替える近接範囲（ワールド距離）。
    static constexpr float nearbyAttractionRange = 3.0f;

    // 境界付近で所属惑星が細かく切り替わるのを防ぐ距離差。
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
    bool mIsNearbySurfaceAttractionPullActive = false;
    bool mShouldPreservePlatformTakeoffDirection = false;
    Planet* mLastLandedPlanet = nullptr;
    glm::vec3 mOverheadGravityUpDirection{0.0f, 1.0f, 0.0f};
    glm::vec3 mPlatformTakeoffUpDirection{0.0f, 1.0f, 0.0f};

    // Actor::UpdateUpVecによる毎フレームの書き換えに影響されないよう、
    // 補間中の上方向をこのクラスで保持する。
    bool mSmoothedUpInitialized = false;
    glm::vec3 mSmoothedUpVec{0.0f, 1.0f, 0.0f};
};
