#include "component/DestructibleComponent.h"

#include "Game.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "system/AudioSystem.h"

#include <cmath>

DestructibleComponent::DestructibleComponent(Actor* owner, int updateOrder)
    : Component(owner, updateOrder),
      mIsDestroyed(false),
      mDestroyHp(0.0f)
{
}

void DestructibleComponent::Update(float deltaTime)
{
    if (mIsDestroyed || !mOwner || !mOwner->GetGame()) {
        return;
    }

    TryApplyDamage();
}

void DestructibleComponent::TryApplyDamage()
{
    for (Player* player : mOwner->GetGame()->GetPlayers()) {
        if (!player) {
            continue;
        }

        const bool isAttacking = player->IsAttacking();
        const std::uint64_t attackSequence = player->GetResolvedAttackSequence();

        // 初回の待機状態は過去の攻撃として閉じ、次の攻撃開始で再利用しない。
        const auto [hitStateIt, wasInserted] =
            mPlayerAttackHitStates.try_emplace(player, PlayerAttackHitState{attackSequence, !isAttacking});
        PlayerAttackHitState& hitState = hitStateIt->second;
        const bool didResolveAttack = !wasInserted && hitState.attackSequence != attackSequence;
        if (didResolveAttack) {
            hitState = {attackSequence, false};
        }

        // 予約中は前回の攻撃番号が残る。番号0も未解決なので、どちらも命中判定に使わない。
        if (attackSequence == 0 || player->HasPendingAttackHit() || hitState.isConsumed) {
            continue;
        }

        // 叩き落としは解決と同時にIdleへ戻るため、番号が進んだ更新では攻撃中フラグに依存しない。
        if (!isAttacking && !didResolveAttack) {
            hitState.isConsumed = true;
            continue;
        }

        if (!IsPlayerInHitRange(player)) {
            continue;
        }

        // 他Playerの待機や攻撃終了では、このPlayerの命中履歴を解除しない。
        hitState.isConsumed = true;
        ApplyDamage(player->CalculateOutgoingAttackDamage(player->GetAttack()));
    }
}

bool DestructibleComponent::IsPlayerInHitRange(Player* player) const
{
    const glm::vec3 toOwner = mOwner->GetPos() - player->GetPos();
    const float distTo = glm::length(toOwner);

    constexpr float hitRangeMargin = 2.0f;
    const float hitRange = mOwner->GetRadius() + hitRangeMargin;

    const glm::vec3 toOwnerN = glm::normalize(toOwner);
    const float dot = glm::dot(player->GetFacingForwardVec(), toOwnerN);
    const float threshold = std::cos(player->GetAttackAngle() * 0.5f);

    return distTo <= hitRange && dot >= threshold;
}

void DestructibleComponent::ApplyDamage(float attack)
{
    mDestroyHp -= attack;

    if (mDestroyHp > 0.0f) {
        mOwner->GetGame()->GetAudioSystem()->PlaySE("attack_se");
        return;
    }

    mIsDestroyed = true;
}
