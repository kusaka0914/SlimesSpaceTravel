#include "support/TestSupport.h"

#include "actor/NPCTalkData.h"

#include <string>

namespace {

void RemovingPageKeepsRemainingMetadataTogether()
{
    NPCTalkData data;
    data.AddTalkTexts("first");
    data.AddTalkTexts("second");
    data.SetTalkCameraFocusTarget(1, "stage", 7);
    data.SetTalkStageClearCondition(1, 3);
    data.SetTalkAdvanceCondition(1, TalkPageAdvanceCondition::Jump);
    data.SetTalkStartsOpeningAfterPage(1, true);
    data.SetTalkStartsEndingAfterPage(1, true);
    data.SetTalkRubySegments(1, {{"second", "reading", true}});
    data.SetTalkProximityMessageText(1, "message");
    data.SetTalkProximityMessageRubySegments(1, {{"message", "reading", true}});

    data.RemoveTalkText(0);

    ExpectEqual(std::size_t{1}, data.GetTalkTexts().size(), "remaining page count");
    ExpectEqual(std::string("second"), data.GetTalkTexts()[0], "remaining text");
    ExpectEqual(3, data.GetTalkStageClearCondition(0), "stage condition moves with page");
    const auto* focus = data.GetTalkCameraFocusTarget(0);
    ExpectTrue(focus != nullptr, "focus target remains available");
    ExpectEqual(std::string("stage"), focus->sequenceName, "focus sequence moves with page");
    ExpectEqual(7, focus->yamlIndex, "focus index moves with page");
    ExpectTrue(data.GetTalkAdvanceCondition(0) == TalkPageAdvanceCondition::Jump, "advance condition moves with page");
    ExpectTrue(data.GetTalkStartsOpeningAfterPage(0), "opening flag moves with page");
    ExpectTrue(data.GetTalkStartsEndingAfterPage(0), "ending flag moves with page");
    ExpectTrue(data.HasValidTalkRuby(0), "ruby moves with text");
    ExpectTrue(data.HasValidTalkProximityMessageRuby(0), "message and ruby move together");

    data.AddTalkTexts("new");
    ExpectEqual(-1, data.GetTalkStageClearCondition(1), "new page does not inherit removed metadata");
    ExpectTrue(data.GetTalkCameraFocusTarget(1) == nullptr, "new page has no focus target");
    ExpectFalse(data.GetTalkStartsEndingAfterPage(1), "new page has no ending flag");
}

void TextChangesInvalidateOnlyTheirOwnRuby()
{
    NPCTalkData data;
    data.AddTalkTexts("text");
    data.SetTalkRubySegments(0, {{"text", "reading", true}});
    data.SetTalkProximityMessageText(0, "message");
    data.SetTalkProximityMessageRubySegments(0, {{"message", "reading", true}});

    data.SetTalkText(0, "text");
    ExpectTrue(data.HasValidTalkRuby(0), "identical text preserves ruby");
    data.SetTalkText(0, "changed");
    ExpectTrue(data.GetTalkRubySegments(0).empty(), "changed text clears obsolete ruby");
    ExpectTrue(data.HasValidTalkProximityMessageRuby(0), "message ruby is independent of page text");

    data.SetTalkProximityMessageText(0, "changed message");
    ExpectTrue(data.GetTalkProximityMessageRubySegments(0).empty(), "changed message clears obsolete message ruby");
}

void ProximityMessagesNormalizeNewlinesBeforeComparingRuby()
{
    NPCTalkData data;
    data.AddTalkTexts("page");
    data.SetTalkProximityMessageText(0, "a\nb\\nc\rd");
    ExpectEqual(std::string("a b c d"), data.GetTalkProximityMessageText(0), "all newline forms become spaces");
    data.SetTalkProximityMessageRubySegments(0, {{"a b c d", "reading", true}});

    data.SetTalkProximityMessageText(0, "a b c d");
    ExpectTrue(data.HasValidTalkProximityMessageRuby(0), "equivalent normalized text keeps ruby");
    data.SetTalkProximityMessageRubyReading(0, 0, "updated");
    ExpectEqual(std::string("updated"), data.GetTalkProximityMessageRubySegments(0)[0].reading,
                "reading can be edited");
}

void OutOfRangeEditsDoNotCreatePagesOrChangeExistingPages()
{
    NPCTalkData data;
    data.AddTalkTexts("page");
    data.SetTalkStageClearCondition(0, -5);
    data.SetTalkText(7, "invalid");
    data.RemoveTalkText(7);
    data.SetTalkStageClearCondition(7, 3);
    data.SetTalkCameraFocusTarget(7, "stage", 2);
    data.SetTalkAdvanceCondition(7, TalkPageAdvanceCondition::PlayerSwitch);
    data.SetTalkStartsOpeningAfterPage(7, true);
    data.SetTalkStartsEndingAfterPage(7, true);
    data.SetTalkRubySegments(7, {{"invalid", "reading", true}});
    data.SetTalkRubyReading(7, 0, "invalid");
    data.SetTalkProximityMessageText(7, "invalid");
    data.SetTalkProximityMessageRubySegments(7, {{"invalid", "reading", true}});
    data.SetTalkProximityMessageRubyReading(7, 0, "invalid");
    data.ClearTalkCameraFocusTarget(7);
    data.ClearTalkRubySegments(7);
    data.ClearTalkProximityMessageRubySegments(7);

    ExpectEqual(std::size_t{1}, data.GetTalkTexts().size(), "invalid edits do not grow data");
    ExpectEqual(std::string("page"), data.GetTalkTexts()[0], "valid page is unchanged");
    ExpectEqual(-1, data.GetTalkStageClearCondition(0), "negative stage number means no condition");
    ExpectEqual(-1, data.GetTalkStageClearCondition(7), "invalid condition has default value");
    ExpectTrue(data.GetTalkCameraFocusTarget(7) == nullptr, "invalid focus is absent");
    ExpectTrue(data.GetTalkAdvanceCondition(7) == TalkPageAdvanceCondition::Confirm, "invalid advance uses confirm");
    ExpectFalse(data.GetTalkStartsOpeningAfterPage(7), "invalid opening flag is false");
    ExpectFalse(data.GetTalkStartsEndingAfterPage(7), "invalid ending flag is false");
    ExpectTrue(data.GetTalkRubySegments(7).empty(), "invalid ruby is empty");
    ExpectFalse(data.HasValidTalkRuby(7), "invalid ruby is not valid");
    ExpectTrue(data.GetTalkProximityMessageText(7).empty(), "invalid message is empty");
    ExpectTrue(data.GetTalkProximityMessageRubySegments(7).empty(), "invalid message ruby is empty");
    ExpectFalse(data.HasValidTalkProximityMessageRuby(7), "invalid message ruby is not valid");
}

void RubyMustMatchTheCurrentBaseText()
{
    NPCTalkData data;
    data.AddTalkTexts("hello");
    data.SetTalkRubySegments(0, {{"wrong", "reading", true}});
    ExpectFalse(data.HasValidTalkRuby(0), "mismatched page ruby is rejected");
    data.SetTalkRubySegments(0, {{"he", "first", true}, {"llo", "second", true}});
    ExpectTrue(data.HasValidTalkRuby(0), "all segment base text is considered");
    data.SetTalkRubyReading(0, 1, "updated");
    ExpectEqual(std::string("updated"), data.GetTalkRubySegments(0)[1].reading, "reading edits preserve base text");
    data.SetTalkRubyReading(0, 9, "invalid");
    ExpectTrue(data.HasValidTalkRuby(0), "invalid segment edits are ignored");
}

} // namespace

void RegisterNPCTalkDataTests(TestRegistry& tests)
{
    tests.emplace_back("NPCTalkData.RemovingPageKeepsRemainingMetadataTogether",
                       RemovingPageKeepsRemainingMetadataTogether);
    tests.emplace_back("NPCTalkData.TextChangesInvalidateOnlyTheirOwnRuby", TextChangesInvalidateOnlyTheirOwnRuby);
    tests.emplace_back("NPCTalkData.ProximityMessagesNormalizeNewlinesBeforeComparingRuby",
                       ProximityMessagesNormalizeNewlinesBeforeComparingRuby);
    tests.emplace_back("NPCTalkData.OutOfRangeEditsDoNotCreatePagesOrChangeExistingPages",
                       OutOfRangeEditsDoNotCreatePagesOrChangeExistingPages);
    tests.emplace_back("NPCTalkData.RubyMustMatchTheCurrentBaseText", RubyMustMatchTheCurrentBaseText);
}
