#include "actor/player/PlayerCombatInput.h"
#include "support/TestSupport.h"

namespace {
void WeakAttackStartsImmediatelyAndLongPressFiresOnce()
{
    PlayerCombatInput input;
    auto request = input.Update(true, false, false, false, false, 0.1f);
    ExpectTrue(request.weakAttack, "Y press starts weak attack immediately");
    ExpectFalse(request.continuousAttack, "short hold does not start continuous attack");
    request = input.Update(true, false, false, false, false, 0.59f);
    ExpectFalse(request.weakAttack || request.continuousAttack, "no repeat before threshold");
    request = input.Update(true, false, false, false, false, 0.01f);
    ExpectTrue(request.continuousAttack, "0.7 seconds starts continuous attack");
    ExpectFalse(request.weakAttack, "long-press transition does not add a weak attack");
    ExpectFalse(input.Update(true, false, false, false, false, 3.0f).continuousAttack, "held Y does not restart");
    input.Update(false, false, false, false, false, 0.1f);
    ExpectTrue(input.Update(true, false, false, false, false, 0.7f).continuousAttack, "release rearms Y");
}

void StrongAttackWaitsForShortRelease()
{
    PlayerCombatInput input;
    ExpectFalse(input.Update(false, true, false, false, false, 0.69f).strongAttack, "X press waits");
    const auto request = input.Update(false, false, false, false, false, 0.01f);
    ExpectTrue(request.strongAttack, "short X release starts strong attack");
    ExpectFalse(request.chargeAttack || request.recover, "short X has one action");
    ExpectFalse(input.Update(false, false, false, false, false, 0.1f).strongAttack, "release only once");
}

void StrongLongPressSuppressesReleaseAction()
{
    for (bool assist : {false, true}) {
        PlayerCombatInput input;
        const auto request = input.Update(false, true, false, assist, false, 0.7f);
        ExpectTrue(request.chargeAttack, "exact threshold charges in either style");
        ExpectFalse(request.strongAttack || request.recover, "long X has no short action");
        ExpectFalse(input.Update(false, true, false, assist, false, 1.0f).chargeAttack, "no repeated charge");
        const auto released = input.Update(false, false, false, assist, false, 0.1f);
        ExpectFalse(released.strongAttack || released.recover, "failed or completed charge has no short release");
    }
}

void RecoveryHasDedicatedPressAndAssistShortRelease()
{
    PlayerCombatInput input;
    ExpectTrue(input.Update(false, false, true, false, false, 0.01f).recover, "L heals on press");
    ExpectFalse(input.Update(false, false, true, false, false, 1.0f).recover, "L does not repeat");
    input.Update(false, true, false, true, false, 0.2f);
    const auto request = input.Update(false, false, false, true, false, 0.1f);
    ExpectTrue(request.recover, "assist short X heals");
    ExpectFalse(request.strongAttack || request.chargeAttack, "assist X does not attack");
}

void ChargedFireRequiresReleaseAndRepress()
{
    PlayerCombatInput input;
    input.Update(false, true, false, true, false, 0.7f);
    ExpectFalse(input.Update(false, true, false, true, true, 5.0f).fireChargedAttack, "initial hold never fires");
    auto request = input.Update(false, false, false, true, true, 0.1f);
    ExpectFalse(request.recover || request.strongAttack, "charging release has no short action");
    request = input.Update(false, true, true, true, true, 0.1f);
    ExpectTrue(request.fireChargedAttack, "repress fires");
    ExpectFalse(request.recover || request.chargeAttack, "charge input cannot heal or recharge");
    request = input.Update(false, false, false, true, false, 0.1f);
    ExpectFalse(request.recover || request.strongAttack, "fire release has no short action");
}

void SuppressionDropsPendingShortAndLongActions()
{
    PlayerCombatInput input;
    input.Update(true, true, false, false, false, 0.6f);
    input.SuppressUntilReleased();
    auto request = input.Update(true, true, true, false, false, 1.0f);
    ExpectFalse(request.weakAttack || request.chargeAttack || request.continuousAttack || request.recover,
                "held controls are suppressed");
    request = input.Update(false, false, false, false, false, 0.1f);
    ExpectFalse(request.strongAttack || request.recover, "interrupted release has no action");
    ExpectTrue(input.Update(true, false, false, false, false, 0.1f).weakAttack, "new press rearms after release");
}

void PlayerHoldTimersAreIndependent()
{
    PlayerCombatInput first;
    PlayerCombatInput second;
    first.Update(true, false, false, false, false, 0.6f);
    second.Update(false, true, false, true, false, 0.2f);
    ExpectTrue(first.Update(true, false, false, false, false, 0.1f).continuousAttack, "P1 reaches threshold");
    ExpectTrue(second.Update(false, false, false, true, false, 0.1f).recover, "P2 remains short press");
}

void OldModifierCombinationsRemainIndependentActions()
{
    PlayerCombatInput input;
    const auto request = input.Update(true, false, true, false, false, 0.1f);
    ExpectTrue(request.weakAttack && request.recover, "L and Y remain separate weak and recovery inputs");
    ExpectFalse(request.continuousAttack || request.chargeAttack, "L no longer selects special attacks");
}
} // namespace

void RegisterPlayerCombatInputTests(TestRegistry& tests)
{
    tests.emplace_back("PlayerCombatInput.WeakImmediateAndLongOnce", WeakAttackStartsImmediatelyAndLongPressFiresOnce);
    tests.emplace_back("PlayerCombatInput.StrongShortRelease", StrongAttackWaitsForShortRelease);
    tests.emplace_back("PlayerCombatInput.StrongLongReleaseSuppressed", StrongLongPressSuppressesReleaseAction);
    tests.emplace_back("PlayerCombatInput.DedicatedAndAssistRecovery", RecoveryHasDedicatedPressAndAssistShortRelease);
    tests.emplace_back("PlayerCombatInput.ChargedReleaseAndRepress", ChargedFireRequiresReleaseAndRepress);
    tests.emplace_back("PlayerCombatInput.Suppression", SuppressionDropsPendingShortAndLongActions);
    tests.emplace_back("PlayerCombatInput.IndependentPlayers", PlayerHoldTimersAreIndependent);
    tests.emplace_back("PlayerCombatInput.NoModifierCombinations", OldModifierCombinationsRemainIndependentActions);
}
