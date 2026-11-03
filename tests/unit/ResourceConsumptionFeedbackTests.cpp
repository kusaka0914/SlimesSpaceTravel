#include "gfx/ui/ResourceConsumptionFeedback.h"
#include "support/TestSupport.h"

namespace {
void ConsumptionHighlightsOnlyLostSlotsAndExpires()
{
    ResourceConsumptionFeedback feedback;
    feedback.Update(6, 6, 0.0f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.0f, "initial sample does not flash");
    feedback.Update(4, 6, 0.01f, true);
    ExpectEqual(4, feedback.GetFirstSlot(), "first lost heart index");
    ExpectEqual(6, feedback.GetEndSlot(), "last lost heart boundary");
    ExpectNear(1.0f, feedback.GetStrength(), 0.0f, "loss starts full strength");
    feedback.Update(4, 6, 0.1f, true);
    ExpectNear(0.5f, feedback.GetStrength(), 0.001f, "same timing for every HUD resource");
    feedback.Update(4, 6, 0.1f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.001f, "flash ends after 0.2 seconds");
    feedback.Update(6, 6, 0.1f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.0f, "recovery does not cause damage feedback");
}

void GuardConsumptionDoesNotTriggerHpFeedback()
{
    ResourceConsumptionFeedback hp;
    ResourceConsumptionFeedback guard;
    hp.Update(6, 6, 0.0f, true);
    guard.Update(3, 3, 0.0f, true);
    hp.Update(6, 6, 0.01f, true);
    guard.Update(2, 3, 0.01f, true);
    ExpectNear(0.0f, hp.GetStrength(), 0.0f, "guarded hit or invincibility does not flash red");
    ExpectNear(1.0f, guard.GetStrength(), 0.0f, "only shield flashes");
    ExpectEqual(2, guard.GetFirstSlot(), "consumed shield slot");
}

void ZeroBalanceAndConsecutiveConsumptionKeepTheirRanges()
{
    ResourceConsumptionFeedback jewels;
    jewels.Update(2, 6, 0.0f, true);
    jewels.Update(1, 6, 0.01f, true);
    jewels.Update(0, 6, 0.05f, true);
    ExpectEqual(0, jewels.GetFirstSlot(), "zero balance is still highlighted");
    ExpectEqual(2, jewels.GetEndSlot(), "consecutive loss retains both consumed slots");
    jewels.Update(0, 6, 0.2f, true);
    ExpectNear(0.0f, jewels.GetStrength(), 0.001f, "zero balance settles to dark icons");
}

void HiddenAndResetStatesDoNotReplayOldConsumption()
{
    ResourceConsumptionFeedback feedback;
    feedback.Update(6, 6, 0.0f, true);
    feedback.Update(4, 6, 0.01f, false);
    feedback.Update(4, 6, 0.01f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.0f, "menu or reload loss is not replayed");
    feedback.Update(3, 6, 0.01f, true);
    feedback.Reset();
    feedback.Update(2, 6, 0.01f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.0f, "stage reset discards old balance");
    feedback.Update(1, 3, 0.01f, true);
    ExpectNear(0.0f, feedback.GetStrength(), 0.0f, "maximum change is not treated as consumption");
}
} // namespace

void RegisterResourceConsumptionFeedbackTests(TestRegistry& tests)
{
    tests.emplace_back("ResourceConsumptionFeedback.LostSlotsAndTiming", ConsumptionHighlightsOnlyLostSlotsAndExpires);
    tests.emplace_back("ResourceConsumptionFeedback.GuardDoesNotDamageHp", GuardConsumptionDoesNotTriggerHpFeedback);
    tests.emplace_back("ResourceConsumptionFeedback.ZeroAndConsecutive",
                       ZeroBalanceAndConsecutiveConsumptionKeepTheirRanges);
    tests.emplace_back("ResourceConsumptionFeedback.HiddenAndReset", HiddenAndResetStatesDoNotReplayOldConsumption);
}
