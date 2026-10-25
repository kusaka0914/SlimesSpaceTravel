#pragma once

#include "component/Component.h"
#include "component/PlatformRevealTarget.h"

#include <cstddef>
#include <string>
#include <vector>

class Actor;
class Platform;
class Player;

// 同じgroupIdの異なる2つの床を別々のPlayerが同時に押すと成立し、対象の出現・非表示を切り替える。
// 成立状態は押下がなくなっても維持する。グループ判定は代表のComponentだけが行う。
class PlatformLatchedGroupSwitchComponent : public Component {
public:
    explicit PlatformLatchedGroupSwitchComponent(Platform* owner, int updateOrder = 86);
    ~PlatformLatchedGroupSwitchComponent() override;

    void Update(float deltaTime) override;

    // IDが変わる場合、旧対象への制御要求を解除し、このComponentの押下・成立・演出状態をリセットする。
    void SetGroupId(const std::string& groupId);
    const std::string& GetGroupId() const { return mGroupId; }

    // 旧対象への要求を解除して差し替える。無効・自己参照・同じシーケンス名と添字の重複を除く。
    void SetRevealTargets(const std::vector<PlatformRevealTarget>& revealTargets);
    const std::vector<PlatformRevealTarget>& GetRevealTargets() const { return mRevealTargets; }

    // 成立時に非表示にする対象を差し替える。旧要求を解除し、無効・自己参照・重複を除く。
    void SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const { return mHideTargets; }

    bool GetIsOn() const { return mIsGroupActivated || mCurrentPressingPlayer != nullptr; }

    // グループに2つ以上の床があり、そのいずれかが成立済みならtrue。現在の同時押下の有無とは区別する。
    bool GetIsGroupCompleted() const;

    // このComponentが記録した対象への制御要求だけを解除する。成立状態は戻さず、破棄前にも呼ぶこと。
    void ClearTargetRuntimeStates();

private:
    std::vector<PlatformLatchedGroupSwitchComponent*> CollectGroupSwitches() const;
    std::vector<PlatformRevealTarget>
    CollectGroupRevealTargets(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    std::vector<PlatformRevealTarget>
    CollectGroupHideTargets(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    bool IsGroupCoordinator(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    void RefreshCurrentPressingPlayers(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches);
    bool HasRequiredSimultaneousPresses(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches) const;
    void ActivateGroup(const std::vector<PlatformLatchedGroupSwitchComponent*>& groupSwitches);
    Actor* FindTargetActor(const PlatformRevealTarget& target) const;
    void ApplyGroupTargetState(const std::vector<PlatformRevealTarget>& revealTargets,
                               const std::vector<PlatformRevealTarget>& hideTargets, bool isGroupActivated);
    void UpdateFirstActivationReveal(const std::vector<PlatformRevealTarget>& revealTargets);

    static constexpr std::size_t requiredSwitchCount = 2;

    Platform* mPlatform = nullptr;
    std::string mGroupId;
    std::vector<PlatformRevealTarget> mRevealTargets;
    std::vector<PlatformRevealTarget> mHideTargets;
    std::vector<Actor*> mRuntimeTargetActors;
    Player* mCurrentPressingPlayer = nullptr;
    bool mIsGroupActivated = false;
    bool mHasAppliedActivatedTargetState = false;
    bool mIsWaitingForActivationReveal = false;
};
