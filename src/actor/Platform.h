#pragma once

#include "actor/Actor.h"

#include <string>
#include <unordered_map>

class Game;
class Component;
class PlatformConveyorComponent;
class PlatformAdhesionComponent;
class PlatformDirectionalMovementComponent;
class PlatformEnemyClearUnlockComponent;
class PlatformFadeOnStandComponent;
class FocusComponent;
class PlatformIntervalToggleComponent;
class PlatformJumpToggleComponent;
class PlatformLatchedGroupSwitchComponent;
class PlatformMovementComponent;
class PlatformPressureSwitchComponent;
class PlatformRotationComponent;

// 移動床やスイッチの窓口。振る舞いはComponentに任せ、描画・衝突状態と床の移動量を集約する。
class Platform : public Actor {
public:
    Platform(Game* game);
    void UpdateActor(float deltaTime) override;
    bool ShouldUpdateUpVecEveryFrame() const override { return false; }
    bool ShouldRebuildDirectionVectorsEveryFrame() const override { return false; }

    // Componentの追加は冪等。同じ機能を再追加しても既存の参照を返す。所有者はActor。
    // Remove…後はGet…がnullptrを返し、それまでの参照は無効になる。
    PlatformMovementComponent* AddMovementComponent();
    void RemoveMovementComponent();
    PlatformMovementComponent* GetMovementComponent() const { return mMovementComponent; }

    PlatformFadeOnStandComponent* AddFadeOnStandComponent();
    void RemoveFadeOnStandComponent();
    PlatformFadeOnStandComponent* GetFadeOnStandComponent() const { return mFadeOnStandComponent; }

    PlatformJumpToggleComponent* AddJumpToggleComponent();
    void RemoveJumpToggleComponent();
    PlatformJumpToggleComponent* GetJumpToggleComponent() const { return mJumpToggleComponent; }

    PlatformIntervalToggleComponent* AddIntervalToggleComponent();
    void RemoveIntervalToggleComponent();
    PlatformIntervalToggleComponent* GetIntervalToggleComponent() const { return mIntervalToggleComponent; }

    PlatformDirectionalMovementComponent* AddDirectionalMovementComponent();
    void RemoveDirectionalMovementComponent();
    PlatformDirectionalMovementComponent* GetDirectionalMovementComponent() const
    {
        return mDirectionalMovementComponent;
    }

    PlatformRotationComponent* AddRotationComponent();
    void RemoveRotationComponent();
    PlatformRotationComponent* GetRotationComponent() const { return mRotationComponent; }

    PlatformConveyorComponent* AddConveyorComponent();
    void RemoveConveyorComponent();
    PlatformConveyorComponent* GetConveyorComponent() const { return mConveyorComponent; }

    PlatformAdhesionComponent* AddAdhesionComponent();
    void RemoveAdhesionComponent();
    PlatformAdhesionComponent* GetAdhesionComponent() const { return mAdhesionComponent; }

    PlatformPressureSwitchComponent* AddPressureSwitchComponent();
    void RemovePressureSwitchComponent();
    PlatformPressureSwitchComponent* GetPressureSwitchComponent() const { return mPressureSwitchComponent; }

    PlatformEnemyClearUnlockComponent* AddEnemyClearUnlockComponent();
    void RemoveEnemyClearUnlockComponent();
    PlatformEnemyClearUnlockComponent* GetEnemyClearUnlockComponent() const { return mEnemyClearUnlockComponent; }

    PlatformLatchedGroupSwitchComponent* AddLatchedGroupSwitchComponent();
    void RemoveLatchedGroupSwitchComponent();
    PlatformLatchedGroupSwitchComponent* GetLatchedGroupSwitchComponent() const { return mLatchedGroupSwitchComponent; }

    void StartFocus();
    FocusComponent* GetFocusComponent() const { return mFocusComponent; }

    void SetPlatformId(const std::string& platformId) { mPlatformId = platformId; }
    const std::string& GetPlatformId() const { return mPlatformId; }

    void SetUGCGeneratedLayer(int gridLayer)
    {
        mIsUGCGenerated = true;
        mUGCGridLayer = gridLayer;
    }
    bool GetIsUGCGenerated() const { return mIsUGCGenerated; }
    int GetUGCGridLayer() const { return mUGCGridLayer; }

    // Component別の要求を集約する。不透明度は最小値、衝突は1つでもfalseなら無効。
    // ClearComponentRuntimeStateは、指定したComponentの要求だけを解除する。
    void SetComponentOpacity(const Component* component, float opacity);
    void SetComponentCollisionEnabled(const Component* component, bool enabled);
    void ClearComponentRuntimeState(const Component* component);
    void SetConveyorFrameDelta(const glm::vec3& delta) { mConveyorFrameDelta = delta; }

    float GetRenderOpacity() const override;
    glm::vec2 GetRenderTextureTiling() const override;
    const std::string& GetRenderTextureOverridePath() const override;
    bool GetCollisionEnabled() const;
    bool UsesKinematicPhysics() const;

    // ワールド距離でのフレーム変位。UpdateActorで記録し、Component更新後の値から計算する。
    glm::vec3 GetFrameDelta() const;

    // 指定したワールド位置が床の平行移動・回転に追従した場合の変位。コンベヤー移動は含まない。
    glm::vec3 GetTransformFrameDelta(const glm::vec3& worldPoint) const;
    const glm::vec3& GetConveyorFrameDelta() const { return mConveyorFrameDelta; }

private:
    PlatformMovementComponent* mMovementComponent = nullptr;
    PlatformFadeOnStandComponent* mFadeOnStandComponent = nullptr;
    PlatformJumpToggleComponent* mJumpToggleComponent = nullptr;
    PlatformIntervalToggleComponent* mIntervalToggleComponent = nullptr;
    PlatformDirectionalMovementComponent* mDirectionalMovementComponent = nullptr;
    PlatformRotationComponent* mRotationComponent = nullptr;
    PlatformConveyorComponent* mConveyorComponent = nullptr;
    PlatformAdhesionComponent* mAdhesionComponent = nullptr;
    PlatformPressureSwitchComponent* mPressureSwitchComponent = nullptr;
    PlatformEnemyClearUnlockComponent* mEnemyClearUnlockComponent = nullptr;
    PlatformLatchedGroupSwitchComponent* mLatchedGroupSwitchComponent = nullptr;
    FocusComponent* mFocusComponent = nullptr;

    std::string mPlatformId;
    bool mIsUGCGenerated = false;
    int mUGCGridLayer = 0;

    std::unordered_map<const Component*, float> mComponentOpacities;
    std::unordered_map<const Component*, bool> mComponentCollisionStates;

    glm::vec3 mFrameStartPos{0.0f};
    glm::quat mFrameStartOrientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 mConveyorFrameDelta{0.0f};
};
