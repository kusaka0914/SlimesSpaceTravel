#include "support/TestSupport.h"

#include "actor/player/PlayerJewelGauge.h"

namespace {

void ItemPickupDoesNotExceedSixJewels()
{
    PlayerJewelGauge jewelGauge;

    jewelGauge.AddFromItem(100);

    ExpectEqual(6, jewelGauge.GetMaxCount(), "maximum jewel count");
    ExpectEqual(6, jewelGauge.GetCount(), "item pickup is clamped to maximum");
}

void AutomaticRecoveryStopsAtTwoJewels()
{
    PlayerJewelGauge jewelGauge;
    jewelGauge.SetCount(1);

    jewelGauge.Add(100);

    ExpectEqual(2, jewelGauge.GetCount(), "automatic recovery maximum");
}

void ContinuousAttackRequiresAndConsumesTwoJewels()
{
    PlayerJewelGauge jewelGauge;
    jewelGauge.SetCount(1);

    ExpectFalse(jewelGauge.CanConsume(PlayerJewelGauge::ContinuousAttackCost),
                "one jewel cannot start continuous attack");

    jewelGauge.SetCount(2);
    ExpectTrue(jewelGauge.Consume(PlayerJewelGauge::ContinuousAttackCost), "two jewels can start continuous attack");
    ExpectEqual(0, jewelGauge.GetCount(), "continuous attack consumes two jewels");
}

} // namespace

void RegisterPlayerJewelGaugeTests(TestRegistry& tests)
{
    tests.emplace_back("PlayerJewelGauge.ItemPickupDoesNotExceedSixJewels", ItemPickupDoesNotExceedSixJewels);
    tests.emplace_back("PlayerJewelGauge.AutomaticRecoveryStopsAtTwoJewels", AutomaticRecoveryStopsAtTwoJewels);
    tests.emplace_back("PlayerJewelGauge.ContinuousAttackRequiresAndConsumesTwoJewels",
                       ContinuousAttackRequiresAndConsumesTwoJewels);
}
