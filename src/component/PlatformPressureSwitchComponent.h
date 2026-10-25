#pragma once

#include "component/Component.h"
#include "component/PlatformRevealTarget.h"

#include <string>
#include <vector>

class Actor;
class Platform;

// Playerの押下で対象の表示・衝突・実行時の有効状態を切り替える。初回の床出現は注目演出に合わせる。
class PlatformPressureSwitchComponent : public Component {
public:
    explicit PlatformPressureSwitchComponent(Platform* owner, int updateOrder = 85);
    ~PlatformPressureSwitchComponent() override;

    void Update(float deltaTime) override;

    // 旧対象への要求を解除し、空ID・自己ID・重複を除いて設定する。現在の押下状態を再判定して対象へ反映する。
    void SetTargetPlatformIds(const std::vector<std::string>& targetPlatformIds);
    const std::vector<std::string>& GetTargetPlatformIds() const { return mTargetPlatformIds; }

    // 旧対象への要求を解除し、有効な指定を重複なしで設定して即時反映する。敵はシーケンス名とYAML添字で解決する。
    void SetTargetEnemyRefs(const std::vector<PlatformRevealTarget>& targetEnemyRefs);
    const std::vector<PlatformRevealTarget>& GetTargetEnemyRefs() const { return mTargetEnemyRefs; }

    // 押下時に非表示にする対象を設定する。旧要求を解除し、無効・自己参照・重複を除いて即時反映する。
    void SetHideTargets(const std::vector<PlatformRevealTarget>& hideTargets);
    const std::vector<PlatformRevealTarget>& GetHideTargets() const { return mHideTargets; }

    // 未押下時の対象床の不透明度を0〜1に制限し、現在の対象状態へ即時反映する。
    void SetInactiveOpacity(float opacity);
    float GetInactiveOpacity() const { return mInactiveOpacity; }

    // falseへ変更すると成立の保持を解除し、現在の接触から押下状態を再判定する。対象へも即時反映する。
    void SetShouldRemainOnAfterPressed(bool shouldRemainOnAfterPressed);
    bool ShouldRemainOnAfterPressed() const { return mShouldRemainOnAfterPressed; }
    bool GetIsPressed() const { return mIsPressed; }

    // 対象の表示・衝突・有効状態に対する、このComponentの要求だけを解除する。破棄前にも呼ぶこと。
    void ClearTargetRuntimeStates();

private:
    Platform* FindTargetPlatform(const std::string& platformId) const;
    Actor* FindTargetActor(const PlatformRevealTarget& target) const;
    void ApplyTargetState();
    void StartFirstActivationFocus();
    void UpdateFirstActivationReveal();

    Platform* mPlatform = nullptr;
    std::vector<std::string> mTargetPlatformIds;
    std::vector<PlatformRevealTarget> mTargetEnemyRefs;
    std::vector<PlatformRevealTarget> mHideTargets;
    float mInactiveOpacity = 0.2f;
    float mContactGraceRemainingSeconds = 0.0f;
    bool mShouldRemainOnAfterPressed = false;
    bool mHasLatchedOn = false;
    bool mIsPressed = false;
    bool mHasStartedActivationFocus = false;
    bool mIsWaitingForActivationReveal = false;
};
