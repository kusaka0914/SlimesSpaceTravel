#pragma once

#include "component/Component.h"

#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Actor;
class Platform;
class Player;

struct PlatformRevealTarget {
    std::string sequenceName;
    int yamlIndex = -1;
    std::string platformId;

    bool IsValid() const
    {
        return !platformId.empty() ||
               (!sequenceName.empty() && yamlIndex >= 0);
    }
};

class PlatformFadeOnStandComponent : public Component {
public:
    explicit PlatformFadeOnStandComponent(Platform* owner, int updateOrder = 70);
    void Update(float deltaTime) override;

    void SetFadeOutDuration(float duration);
    void SetReappearDelay(float delay);
    float GetFadeOutDuration() const { return mFadeOutDuration; }
    float GetReappearDelay() const { return mReappearDelay; }

private:
    enum class FadePhase {
        Visible,
        FadingOut,
        WaitingToReappear,
    };

    Platform* mPlatform = nullptr;
    float mOpacity = 1.0f;
    float mFadeOutDuration = 1.0f;
    float mReappearDelay = 2.0f;
    float mHiddenTimer = 0.0f;
    bool mCollisionEnabled = true;
    FadePhase mFadePhase = FadePhase::Visible;
};

class PlatformJumpToggleComponent : public Component {
public:
    explicit PlatformJumpToggleComponent(Platform* owner, int updateOrder = 71);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    bool GetVisible() const { return mVisible; }

private:
    void ApplyState();

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    std::uint64_t mLastObservedJumpCount = 0;
};

class PlatformIntervalToggleComponent : public Component {
public:
    explicit PlatformIntervalToggleComponent(Platform* owner, int updateOrder = 72);
    void Update(float deltaTime) override;

    void SetInitiallyVisible(bool visible);
    void SetInterval(float interval);
    void SetWarningDuration(float duration);
    void SetBlinkInterval(float interval);

    bool GetInitiallyVisible() const { return mInitiallyVisible; }
    float GetInterval() const { return mInterval; }
    float GetWarningDuration() const { return mWarningDuration; }
    float GetBlinkInterval() const { return mBlinkInterval; }

private:
    void ApplyState(float opacity);

    Platform* mPlatform = nullptr;
    bool mInitiallyVisible = true;
    bool mVisible = true;
    float mTimer = 0.0f;
    float mInterval = 3.0f;
    float mWarningDuration = 1.0f;
    float mBlinkInterval = 0.15f;
};

class PlatformDirectionalMovementComponent : public Component {
public:
    explicit PlatformDirectionalMovementComponent(Platform* owner, int updateOrder = 55);
    void Update(float deltaTime) override;

    void SetSpeed(float speed);
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mTravelDirection{0.0f};
    float mSpeed = 2.0f;
    bool mWasOccupied = false;
};

class PlatformRotationComponent : public Component {
public:
    explicit PlatformRotationComponent(Platform* owner, int updateOrder = 60);
    void Update(float deltaTime) override;

    void SetLocalAxis(const glm::vec3& axis);
    void SetDegreesPerSecond(float speed) { mDegreesPerSecond = speed; }
    const glm::vec3& GetLocalAxis() const { return mLocalAxis; }
    float GetDegreesPerSecond() const { return mDegreesPerSecond; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalAxis{0.0f, 1.0f, 0.0f};
    float mDegreesPerSecond = 45.0f;
};

class PlatformConveyorComponent : public Component {
public:
    explicit PlatformConveyorComponent(Platform* owner, int updateOrder = 80);
    void Update(float deltaTime) override;

    void SetLocalDirection(const glm::vec3& direction);
    void SetSpeed(float speed);
    const glm::vec3& GetLocalDirection() const { return mLocalDirection; }
    float GetSpeed() const { return mSpeed; }

private:
    Platform* mPlatform = nullptr;
    glm::vec3 mLocalDirection{0.0f, 0.0f, 1.0f};
    float mSpeed = 2.0f;
};




class PlatformAdhesionComponent : public Component {
public:
    explicit PlatformAdhesionComponent(Platform* owner, int updateOrder = 90);
    ~PlatformAdhesionComponent() override;

    void Update(float deltaTime) override;
    static bool TryAttachPlayerToAnyPlatformAlongMovement(
        Player& player,
        const glm::vec3& movementStart);
    bool TryAttachPlayerIfTouching(Player& player);
    bool TryAttachPlayerAlongMovement(
        Player& player,
        const glm::vec3& movementStart);
    void ReleaseAttachedPlayers();

private:
    bool DidPlayerMovementTouchPlatform(
        const Player& player,
        const glm::vec3& movementStart) const;

    Platform* mPlatform = nullptr;
    std::unordered_set<Player*> mAttachedPlayers;
    std::unordered_map<Player*, float>
        mPlayerReattachmentCooldownSeconds;
};

class PlatformEnemyClearUnlockComponent : public Component {
public:
    explicit PlatformEnemyClearUnlockComponent(
        Platform* owner,
        int updateOrder = 84);
    ~PlatformEnemyClearUnlockComponent() override;

    void Update(float deltaTime) override;

    bool GetIsUnlocked() const { return mIsUnlocked; }

private:
    bool HasLivingEnemyOnCurrentPlanet() const;
    void ApplyLockedState();
    void ClearLockedState();

    Platform* mPlatform = nullptr;
    bool mIsUnlocked = false;
};

class PlatformPressureSwitchComponent : public Component {
public:
    explicit PlatformPressureSwitchComponent(
        Platform* owner,
        int updateOrder = 85);
    ~PlatformPressureSwitchComponent() override;

    void Update(float deltaTime) override;

    void SetTargetPlatformIds(
        const std::vector<std::string>& targetPlatformIds);
    const std::vector<std::string>& GetTargetPlatformIds() const
    {
        return mTargetPlatformIds;
    }
    void SetTargetEnemyRefs(
        const std::vector<PlatformRevealTarget>& targetEnemyRefs);
    const std::vector<PlatformRevealTarget>& GetTargetEnemyRefs() const
    {
        return mTargetEnemyRefs;
    }
    void SetHideTargets(
        const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const
    {
        return mHideTargets;
    }
    void SetInactiveOpacity(float opacity);
    float GetInactiveOpacity() const { return mInactiveOpacity; }
    void SetShouldRemainOnAfterPressed(
        bool shouldRemainOnAfterPressed);
    bool ShouldRemainOnAfterPressed() const
    {
        return mShouldRemainOnAfterPressed;
    }
    bool GetIsPressed() const { return mIsPressed; }

    void ClearTargetRuntimeStates();

private:
    Platform* FindTargetPlatform(const std::string& platformId) const;
    Actor* FindTargetActor(const PlatformRevealTarget& target) const;
    void ApplyTargetState();

    Platform* mPlatform = nullptr;
    std::vector<std::string> mTargetPlatformIds;
    std::vector<PlatformRevealTarget> mTargetEnemyRefs;
    std::vector<PlatformRevealTarget> mHideTargets;
    float mInactiveOpacity = 0.2f;
    float mContactGraceRemainingSeconds = 0.0f;
    bool mShouldRemainOnAfterPressed = false;
    bool mHasLatchedOn = false;
    bool mIsPressed = false;
};

class PlatformLatchedGroupSwitchComponent : public Component {
public:
    explicit PlatformLatchedGroupSwitchComponent(
        Platform* owner,
        int updateOrder = 86);
    ~PlatformLatchedGroupSwitchComponent() override;

    void Update(float deltaTime) override;

    void SetGroupId(const std::string& groupId);
    const std::string& GetGroupId() const { return mGroupId; }

    void SetRevealTargets(
        const std::vector<PlatformRevealTarget>& revealTargets);
    const std::vector<PlatformRevealTarget>& GetRevealTargets() const
    {
        return mRevealTargets;
    }
    void SetHideTargets(
        const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const
    {
        return mHideTargets;
    }

    bool GetIsOn() const
    {
        return mIsGroupActivated || mCurrentPressingPlayer != nullptr;
    }
    bool GetIsGroupCompleted() const;

    void ClearTargetRuntimeStates();

private:
    std::vector<PlatformLatchedGroupSwitchComponent*>
    CollectGroupSwitches() const;
    std::vector<PlatformRevealTarget>
    CollectGroupRevealTargets(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches) const;
    std::vector<PlatformRevealTarget>
    CollectGroupHideTargets(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches) const;
    bool IsGroupCoordinator(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches) const;
    void RefreshCurrentPressingPlayers(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches);
    bool HasRequiredSimultaneousPresses(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches) const;
    void ActivateGroup(
        const std::vector<PlatformLatchedGroupSwitchComponent*>&
            groupSwitches);
    Actor* FindTargetActor(
        const PlatformRevealTarget& target) const;
    void ApplyGroupTargetState(
        const std::vector<PlatformRevealTarget>& revealTargets,
        const std::vector<PlatformRevealTarget>& hideTargets,
        bool isGroupActivated);

    static constexpr std::size_t RequiredSwitchCount = 2;

    Platform* mPlatform = nullptr;
    std::string mGroupId;
    std::vector<PlatformRevealTarget> mRevealTargets;
    std::vector<PlatformRevealTarget> mHideTargets;
    std::vector<Actor*> mRuntimeTargetActors;
    Player* mCurrentPressingPlayer = nullptr;
    bool mIsGroupActivated = false;
    bool mHasAppliedActivatedTargetState = false;
};
