#pragma once

#include "component/Component.h"
#include "component/PlatformRevealTarget.h"

#include <string>
#include <vector>

class Actor;
class Platform;

class PlatformPressureSwitchComponent : public Component {
public:
    explicit PlatformPressureSwitchComponent(
        Platform* owner,
        int updateOrder = 85);
    ~PlatformPressureSwitchComponent() override;

    void Update(float deltaTime) override;

    void SetTargetPlatformIds(const std::vector<std::string>& targetPlatformIds);
    const std::vector<std::string>& GetTargetPlatformIds() const { return mTargetPlatformIds; }
    void SetTargetEnemyRefs(const std::vector<PlatformRevealTarget>& targetEnemyRefs);
    const std::vector<PlatformRevealTarget>& GetTargetEnemyRefs() const { return mTargetEnemyRefs; }
    void SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const { return mHideTargets; }
    void SetInactiveOpacity(float opacity);
    float GetInactiveOpacity() const { return mInactiveOpacity; }
    void SetShouldRemainOnAfterPressed(bool shouldRemainOnAfterPressed);
    bool ShouldRemainOnAfterPressed() const { return mShouldRemainOnAfterPressed; }
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
