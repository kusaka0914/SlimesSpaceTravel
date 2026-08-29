#pragma once

#include "component/Component.h"
#include "component/PlatformRevealTarget.h"

#include <cstddef>
#include <string>
#include <vector>

class Actor;
class Platform;
class Player;

class PlatformLatchedGroupSwitchComponent : public Component {
public:
    explicit PlatformLatchedGroupSwitchComponent(
        Platform* owner,
        int updateOrder = 86);
    ~PlatformLatchedGroupSwitchComponent() override;

    void Update(float deltaTime) override;

    void SetGroupId(const std::string& groupId);
    const std::string& GetGroupId() const { return mGroupId; }

    void SetRevealTargets(const std::vector<PlatformRevealTarget>& revealTargets);
    const std::vector<PlatformRevealTarget>& GetRevealTargets() const { return mRevealTargets; }
    void SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const { return mHideTargets; }

    bool GetIsOn() const { return mIsGroupActivated || mCurrentPressingPlayer != nullptr; }
    bool GetIsGroupCompleted() const;

    void ClearTargetRuntimeStates();

private:
    std::vector<PlatformLatchedGroupSwitchComponent*> CollectGroupSwitches() const;
    std::vector<PlatformRevealTarget> CollectGroupRevealTargets(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    std::vector<PlatformRevealTarget> CollectGroupHideTargets(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    bool IsGroupCoordinator(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    void RefreshCurrentPressingPlayers(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches);
    bool HasRequiredSimultaneousPresses(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    void ActivateGroup(
        const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches);
    Actor* FindTargetActor(const PlatformRevealTarget& target) const;
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
