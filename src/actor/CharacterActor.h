#pragma once

#include "actor/Actor.h"

#include <btBulletDynamicsCommon.h>

class Game;
class Platform;

// 地面への着地、移動床への追従、重力積分を提供するキャラクター共通基盤。
// 入力や戦闘の判断は派生クラスが担当する。
class CharacterActor : public Actor {
public:
    // rayFrom・rayToはワールド座標。
    struct RayInfo {
        btVector3 rayFrom;
        btVector3 rayTo;
        btCollisionWorld::ClosestRayResultCallback rayCallback;
    };

    CharacterActor(Game* game);

    // 床の変換へ追従し、接地判定、コンベヤー移動の順に適用する。重力積分は含まない。
    void UpdateActor(float deltaTime) override;

    // ワールド位置へ着地し、速度をリセットしてOnLandedを通知する。
    void Land(const glm::vec3& hitPos);

    // 接地フラグだけを解除する。速度や位置は変更しない。
    void NotLand();

    // 下降移動で衝突した後、足元レイと歩行可能角度で支持面を確認して着地を試みる。
    // 壁への接触だけでは着地しない。成功時は床の参照・位置・速度も通常の着地処理で更新する。
    bool TryLandOnSupportingSurface();

    // ワールド空間の方向を正規化して傾斜を判定する。長さ0の入力は歩行不可。
    static bool IsWalkableGroundNormal(const glm::vec3& hitNormal, const glm::vec3& upDirection);

    // 元のスケールと現在のスケールを同時に更新する。描画だけの変形とは区別する。
    void SetBaseScale(const glm::vec3& scale);
    const glm::vec3& GetBaseScale() const { return mBaseScale; }

    bool GetOnGround() const { return mOnGround; }

    Actor* GetGroundActor() const { return mGroundActor; }

    // 床に固定して接地状態にし、速度を0にして通常の着地判定を停止する。
    // Platformは非所有参照。nullptrなら何もしない。
    void AttachToPlatform(Platform* platform);

    // 固定を解除して空中状態に戻し、通常の着地判定を再開する。未固定なら何もしない。
    void DetachFromPlatform();
    bool IsAttachedToPlatform() const { return mAttachedPlatform != nullptr; }
    Platform* GetAttachedPlatform() const { return mAttachedPlatform; }

    const glm::vec3& GetFacingForwardVec() const { return mFacingForwardVec; }

protected:
    // deltaTimeは秒。上方向の逆向きへ加速し、速度と位置の両方を更新する。既定加速度は9.8。
    void ApplyGravity(float deltaTime);
    void ApplyGravity(float deltaTime, float gravityAcceleration);

    // 共通の歩行可能角度判定に加えて、派生クラスが着地対象を除外するためのフック。
    virtual bool ShouldAcceptLandingSurface(Actor* surfaceActor, const glm::vec3& surfaceNormal) const { return true; }

    bool CheckDotAngleSteep(const glm::vec3& hitNormal, const glm::vec3& up) const override;

private:
    enum class LandingRayResolution { NoHit, Landed, AppliedPlanetFallback };

    void JudgeLanding();

    LandingRayResolution ResolveLandingByRay(const glm::vec3& rayOffset, const glm::vec3& hitPosCorrection);

    RayInfo CreateRayInfo(const glm::vec3& rayOffset) const;

    void ApplyGroundActorTransformMovement();
    void ApplyGroundActorConveyorMovement();
    void ApplyExternalGroundMovement(const glm::vec3& movementDelta);

    // 着地判定が成立するたびに呼ばれる。接地継続中も通知される。
    virtual void OnLanded() {}

    bool ShouldUpdateUpVecEveryFrame() const override { return true; }

protected:
    bool mOnGround;
    bool mShouldJudgeLanding;

    glm::vec3 mVelocity;
    glm::vec3 mFacingForwardVec;

    glm::vec3 mBaseScale;

private:
    Actor* mGroundActor = nullptr;
    Platform* mAttachedPlatform = nullptr;
};
