#include "TestSupport.h"

#include "actor/enemy/EnemyGuardGauge.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void OneSegmentUsesConfiguredGuardValue()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(1, 20.0f);

    ExpectNear(20.0f, guardGauge.GetMaxGuard(), 0.0001f, "one segment maximum guard");
    ExpectNear(20.0f, guardGauge.GetCurrentGuard(), 0.0001f, "one segment current guard");
}

void ThreeSegmentsAccumulateContinuousDamage()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);

    ExpectNear(60.0f, guardGauge.GetMaxGuard(), 0.0001f, "three segment maximum guard");
    guardGauge.ApplyDamage(5.0f);
    ExpectNear(55.0f, guardGauge.GetCurrentGuard(), 0.0001f, "guard after combo hit one");
    guardGauge.ApplyDamage(5.0f);
    ExpectNear(50.0f, guardGauge.GetCurrentGuard(), 0.0001f, "guard after combo hit two");
    const EnemyGuardDamageResult finalComboHit = guardGauge.ApplyDamage(10.0f);
    ExpectNear(40.0f, guardGauge.GetCurrentGuard(), 0.0001f, "guard after combo hit three");
    ExpectEqual(1, finalComboHit.brokenSegmentCount, "combo breaks one segment");
    ExpectFalse(finalComboHit.wasFullyBroken, "remaining segments do not fully break guard");
}

void DamageCrossesSegmentBoundariesWithoutDiscardingRemainder()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);
    guardGauge.ApplyDamage(15.0f);

    const EnemyGuardDamageResult damageResult = guardGauge.ApplyDamage(25.0f);

    ExpectNear(20.0f, guardGauge.GetCurrentGuard(), 0.0001f, "cross-segment remaining guard");
    ExpectEqual(2, damageResult.brokenSegmentCount, "cross-segment destroyed segment count");
    ExpectFalse(damageResult.wasFullyBroken, "cross-segment damage leaves final segment");
}

void GuaranteedBreakConsumesFullCurrentSegmentWhenNecessary()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);

    const EnemyGuardDamageResult damageResult =
        guardGauge.ApplyDamageEnsuringCurrentSegmentBreak(10.0f);

    ExpectNear(40.0f, guardGauge.GetCurrentGuard(), 0.0001f, "full current segment is consumed");
    ExpectEqual(1, damageResult.brokenSegmentCount, "one full segment breaks");
}

void GuaranteedBreakKeepsMinimumDamageOverflow()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);
    guardGauge.ApplyDamage(15.0f);

    const EnemyGuardDamageResult damageResult =
        guardGauge.ApplyDamageEnsuringCurrentSegmentBreak(10.0f);

    ExpectNear(35.0f, guardGauge.GetCurrentGuard(), 0.0001f, "minimum damage carries into next segment");
    ExpectEqual(1, damageResult.brokenSegmentCount, "partially depleted segment breaks");
}

void GuardClampsAtZeroAndReportsFullBreakOnlyOnce()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(1, 20.0f);

    const EnemyGuardDamageResult firstDamage = guardGauge.ApplyDamage(30.0f);
    const EnemyGuardDamageResult repeatedDamage = guardGauge.ApplyDamage(10.0f);

    ExpectNear(0.0f, guardGauge.GetCurrentGuard(), 0.0001f, "guard clamps at zero");
    ExpectTrue(firstDamage.wasFullyBroken, "transition to zero reports full guard break");
    ExpectFalse(repeatedDamage.wasFullyBroken, "empty guard does not report another break");
}

void ResetRestoresMaximumGuard()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);
    guardGauge.ApplyDamage(45.0f);

    guardGauge.Reset();

    ExpectNear(60.0f, guardGauge.GetCurrentGuard(), 0.0001f, "reset current guard");
}

void BreakAllConsumesEveryRemainingSegment()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(3, 20.0f);
    guardGauge.ApplyDamage(5.0f);

    const EnemyGuardDamageResult damageResult = guardGauge.BreakAll();

    ExpectNear(0.0f, guardGauge.GetCurrentGuard(), 0.0001f, "break all current guard");
    ExpectEqual(3, damageResult.brokenSegmentCount, "break all remaining segment count");
    ExpectTrue(damageResult.wasFullyBroken, "break all reports full guard break");
}

void ZeroSegmentsRemainSafe()
{
    EnemyGuardGauge guardGauge;
    guardGauge.Configure(0, 20.0f);

    const EnemyGuardDamageResult damageResult = guardGauge.ApplyDamage(5.0f);
    guardGauge.Reset();

    ExpectNear(0.0f, guardGauge.GetMaxGuard(), 0.0001f, "zero segment maximum guard");
    ExpectNear(0.0f, guardGauge.GetCurrentGuard(), 0.0001f, "zero segment current guard");
    ExpectEqual(0, damageResult.brokenSegmentCount, "zero segment broken count");
    ExpectFalse(damageResult.wasFullyBroken, "zero segment full break transition");
}

}

void RegisterEnemyGuardGaugeTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "EnemyGuardGauge.OneSegmentUsesConfiguredGuardValue",
        OneSegmentUsesConfiguredGuardValue);
    tests.emplace_back(
        "EnemyGuardGauge.ThreeSegmentsAccumulateContinuousDamage",
        ThreeSegmentsAccumulateContinuousDamage);
    tests.emplace_back(
        "EnemyGuardGauge.DamageCrossesSegmentBoundariesWithoutDiscardingRemainder",
        DamageCrossesSegmentBoundariesWithoutDiscardingRemainder);
    tests.emplace_back(
        "EnemyGuardGauge.GuaranteedBreakConsumesFullCurrentSegmentWhenNecessary",
        GuaranteedBreakConsumesFullCurrentSegmentWhenNecessary);
    tests.emplace_back(
        "EnemyGuardGauge.GuaranteedBreakKeepsMinimumDamageOverflow",
        GuaranteedBreakKeepsMinimumDamageOverflow);
    tests.emplace_back(
        "EnemyGuardGauge.GuardClampsAtZeroAndReportsFullBreakOnlyOnce",
        GuardClampsAtZeroAndReportsFullBreakOnlyOnce);
    tests.emplace_back(
        "EnemyGuardGauge.ResetRestoresMaximumGuard",
        ResetRestoresMaximumGuard);
    tests.emplace_back(
        "EnemyGuardGauge.BreakAllConsumesEveryRemainingSegment",
        BreakAllConsumesEveryRemainingSegment);
    tests.emplace_back(
        "EnemyGuardGauge.ZeroSegmentsRemainSafe",
        ZeroSegmentsRemainSafe);
}
