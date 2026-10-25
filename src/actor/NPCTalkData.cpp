#include "actor/NPCTalkData.h"

#include <algorithm>
#include <utility>

void NPCTalkData::AddTalkTexts(const std::string& text)
{
    mTexts.push_back(text);
    mPages.emplace_back();
}

void NPCTalkData::RemoveTalkText(std::size_t index)
{
    if (index >= mTexts.size()) {
        return;
    }
    const auto offset = static_cast<std::ptrdiff_t>(index);
    mTexts.erase(mTexts.begin() + offset);
    mPages.erase(mPages.begin() + offset);
}

void NPCTalkData::SetTalkText(std::size_t index, const std::string& text)
{
    if (index >= mTexts.size()) {
        return;
    }
    if (mTexts[index] != text) {
        mPages[index].rubySegments.clear();
    }
    mTexts[index] = text;
}

void NPCTalkData::SetTalkStageClearCondition(std::size_t index, int stageNum)
{
    if (index < mPages.size()) {
        mPages[index].stageClearCondition = stageNum >= 0 ? stageNum : -1;
    }
}

int NPCTalkData::GetTalkStageClearCondition(std::size_t index) const
{
    return index < mPages.size() ? mPages[index].stageClearCondition : -1;
}

void NPCTalkData::SetTalkCameraFocusTarget(std::size_t index, const std::string& sequenceName, int yamlIndex)
{
    if (index < mPages.size()) {
        mPages[index].cameraFocusTarget = NPCTalkCameraFocusTarget{sequenceName, yamlIndex};
    }
}

void NPCTalkData::ClearTalkCameraFocusTarget(std::size_t index)
{
    if (index < mPages.size()) {
        mPages[index].cameraFocusTarget.reset();
    }
}

const NPCTalkCameraFocusTarget* NPCTalkData::GetTalkCameraFocusTarget(std::size_t index) const
{
    if (index >= mPages.size() || !mPages[index].cameraFocusTarget) {
        return nullptr;
    }
    return &*mPages[index].cameraFocusTarget;
}

void NPCTalkData::SetTalkAdvanceCondition(std::size_t index, TalkPageAdvanceCondition condition)
{
    if (index < mPages.size()) {
        mPages[index].advanceCondition = condition;
    }
}

TalkPageAdvanceCondition NPCTalkData::GetTalkAdvanceCondition(std::size_t index) const
{
    return index < mPages.size() ? mPages[index].advanceCondition : TalkPageAdvanceCondition::Confirm;
}

void NPCTalkData::SetTalkStartsOpeningAfterPage(std::size_t index, bool enabled)
{
    if (index < mPages.size()) {
        mPages[index].startsOpeningAfterPage = enabled;
    }
}

bool NPCTalkData::GetTalkStartsOpeningAfterPage(std::size_t index) const
{
    return index < mPages.size() && mPages[index].startsOpeningAfterPage;
}

void NPCTalkData::SetTalkStartsEndingAfterPage(std::size_t index, bool enabled)
{
    if (index < mPages.size()) {
        mPages[index].startsEndingAfterPage = enabled;
    }
}

bool NPCTalkData::GetTalkStartsEndingAfterPage(std::size_t index) const
{
    return index < mPages.size() && mPages[index].startsEndingAfterPage;
}

void NPCTalkData::SetTalkRubySegments(std::size_t index, std::vector<RubyTextSegment> segments)
{
    if (index < mPages.size()) {
        mPages[index].rubySegments = std::move(segments);
    }
}

void NPCTalkData::SetTalkRubyReading(std::size_t talkIndex, std::size_t segmentIndex, const std::string& reading)
{
    if (talkIndex < mPages.size() && segmentIndex < mPages[talkIndex].rubySegments.size()) {
        mPages[talkIndex].rubySegments[segmentIndex].reading = reading;
    }
}

void NPCTalkData::ClearTalkRubySegments(std::size_t index)
{
    if (index < mPages.size()) {
        mPages[index].rubySegments.clear();
    }
}

const std::vector<RubyTextSegment>& NPCTalkData::GetTalkRubySegments(std::size_t index) const
{
    static const std::vector<RubyTextSegment> emptySegments;
    return index < mPages.size() ? mPages[index].rubySegments : emptySegments;
}

bool NPCTalkData::HasValidTalkRuby(std::size_t index) const
{
    return index < mTexts.size() && !mPages[index].rubySegments.empty() &&
           JoinRubyBaseText(mPages[index].rubySegments) == mTexts[index];
}

void NPCTalkData::SetTalkProximityMessageText(std::size_t index, const std::string& text)
{
    if (index >= mPages.size()) {
        return;
    }

    std::string sanitizedText = text;
    std::replace(sanitizedText.begin(), sanitizedText.end(), '\r', ' ');
    std::replace(sanitizedText.begin(), sanitizedText.end(), '\n', ' ');
    std::size_t escapedNewline = std::string::npos;
    while ((escapedNewline = sanitizedText.find("\\n")) != std::string::npos) {
        sanitizedText.replace(escapedNewline, 2, " ");
    }

    PageSettings& page = mPages[index];
    if (page.proximityMessageText != sanitizedText) {
        page.proximityMessageRubySegments.clear();
    }
    page.proximityMessageText = std::move(sanitizedText);
}

const std::string& NPCTalkData::GetTalkProximityMessageText(std::size_t index) const
{
    static const std::string emptyText;
    return index < mPages.size() ? mPages[index].proximityMessageText : emptyText;
}

void NPCTalkData::SetTalkProximityMessageRubySegments(std::size_t index, std::vector<RubyTextSegment> segments)
{
    if (index < mPages.size()) {
        mPages[index].proximityMessageRubySegments = std::move(segments);
    }
}

void NPCTalkData::SetTalkProximityMessageRubyReading(std::size_t talkIndex, std::size_t segmentIndex,
                                                     const std::string& reading)
{
    if (talkIndex < mPages.size() && segmentIndex < mPages[talkIndex].proximityMessageRubySegments.size()) {
        mPages[talkIndex].proximityMessageRubySegments[segmentIndex].reading = reading;
    }
}

void NPCTalkData::ClearTalkProximityMessageRubySegments(std::size_t index)
{
    if (index < mPages.size()) {
        mPages[index].proximityMessageRubySegments.clear();
    }
}

const std::vector<RubyTextSegment>& NPCTalkData::GetTalkProximityMessageRubySegments(std::size_t index) const
{
    static const std::vector<RubyTextSegment> emptySegments;
    return index < mPages.size() ? mPages[index].proximityMessageRubySegments : emptySegments;
}

bool NPCTalkData::HasValidTalkProximityMessageRuby(std::size_t index) const
{
    return index < mPages.size() && !mPages[index].proximityMessageRubySegments.empty() &&
           JoinRubyBaseText(mPages[index].proximityMessageRubySegments) == mPages[index].proximityMessageText;
}

const char* GetTalkPageAdvanceConditionId(TalkPageAdvanceCondition condition)
{
    switch (condition) {
    case TalkPageAdvanceCondition::PlayerSwitch:
        return "playerSwitch";
    case TalkPageAdvanceCondition::Jump:
        return "jump";
    case TalkPageAdvanceCondition::Confirm:
    default:
        return "confirm";
    }
}

TalkPageAdvanceCondition ParseTalkPageAdvanceConditionId(const std::string& conditionId)
{
    if (conditionId == "playerSwitch") {
        return TalkPageAdvanceCondition::PlayerSwitch;
    }
    if (conditionId == "jump") {
        return TalkPageAdvanceCondition::Jump;
    }
    return TalkPageAdvanceCondition::Confirm;
}
