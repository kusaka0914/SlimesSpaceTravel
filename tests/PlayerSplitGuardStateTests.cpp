#include "TestSupport.h"

#include "system/PlayerSplitGuardState.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void StartsFullAndBlocksThreeHits()
{
    PlayerSplitGuardState guardState;

    ExpectEqual(3, guardState.GetCount(), "initial guard count");
    ExpectTrue(guardState.ConsumeOne(), "first guard consumption");
    ExpectTrue(guardState.ConsumeOne(), "second guard consumption");
    ExpectTrue(guardState.ConsumeOne(), "third guard consumption");
    ExpectFalse(guardState.ConsumeOne(), "consumption while empty");
    ExpectEqual(0, guardState.GetCount(), "empty guard count");
}

void RecoversOneGuardEveryTenSeconds()
{
    PlayerSplitGuardState guardState;
    guardState.ConsumeOne();
    guardState.ConsumeOne();
    guardState.ConsumeOne();

    guardState.Update(9.9f);
    ExpectEqual(0, guardState.GetCount(), "guard count before interval");

    guardState.Update(0.1f);
    ExpectEqual(1, guardState.GetCount(), "guard count after first interval");

    guardState.Update(20.0f);
    ExpectEqual(3, guardState.GetCount(), "guard count after three intervals");
}

void AdditionalDamageRestartsRecoveryInterval()
{
    PlayerSplitGuardState guardState;
    guardState.ConsumeOne();
    guardState.Update(8.0f);
    guardState.ConsumeOne();
    guardState.Update(2.0f);

    ExpectEqual(1, guardState.GetCount(), "guard count two seconds after second hit");

    guardState.Update(8.0f);
    ExpectEqual(2, guardState.GetCount(), "guard count ten seconds after second hit");
}

void DamageWhileEmptyRestartsRecoveryInterval()
{
    PlayerSplitGuardState guardState;
    guardState.ConsumeOne();
    guardState.ConsumeOne();
    guardState.ConsumeOne();
    guardState.Update(9.0f);

    ExpectFalse(guardState.ConsumeOne(), "empty guard cannot block damage");
    guardState.Update(1.0f);
    ExpectEqual(0, guardState.GetCount(), "empty guard remains empty one second after damage");

    guardState.Update(9.0f);
    ExpectEqual(1, guardState.GetCount(), "empty guard recovers ten seconds after damage");
}

}

void RegisterPlayerSplitGuardStateTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlayerSplitGuardState.StartsFullAndBlocksThreeHits",
        StartsFullAndBlocksThreeHits);
    tests.emplace_back(
        "PlayerSplitGuardState.RecoversOneGuardEveryTenSeconds",
        RecoversOneGuardEveryTenSeconds);
    tests.emplace_back(
        "PlayerSplitGuardState.AdditionalDamageRestartsRecoveryInterval",
        AdditionalDamageRestartsRecoveryInterval);
    tests.emplace_back(
        "PlayerSplitGuardState.DamageWhileEmptyRestartsRecoveryInterval",
        DamageWhileEmptyRestartsRecoveryInterval);
}
