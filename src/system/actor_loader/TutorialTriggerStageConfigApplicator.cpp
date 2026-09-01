#include "system/actor_loader/TutorialTriggerStageConfigApplicator.h"

#include "actor/TutorialTrigger.h"
#include "system/text/JapaneseRubyGenerator.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace {

void ApplyTalkPageAdvanceConditions(
    NPC* npc,
    const YAML::Node& actorNode)
{
    if (!npc || !actorNode["talkAdvanceConditions"] ||
        !actorNode["talkAdvanceConditions"].IsSequence()) {
        return;
    }

    for (const YAML::Node& conditionNode :
         actorNode["talkAdvanceConditions"]) {
        if (!conditionNode.IsMap() ||
            !conditionNode["talkIndex"] ||
            !conditionNode["condition"]) {
            continue;
        }

        const int talkIndex =
            conditionNode["talkIndex"].as<int>();
        if (talkIndex < 0) {
            continue;
        }

        npc->SetTalkAdvanceCondition(
            static_cast<std::size_t>(talkIndex),
            ParseTalkPageAdvanceConditionId(
                conditionNode["condition"].as<std::string>()));
    }
}


}

void ApplyTutorialTriggerStageConfig(
    TutorialTrigger& configuredTrigger,
    const YAML::Node& triggerNode)
{
    TutorialTrigger* trigger = &configuredTrigger;
            trigger->SetTutorialId(
                triggerNode["tutorialId"]
                    ? triggerNode["tutorialId"].as<std::string>()
                    : std::string());
            trigger->SetRequiredCompletedTutorialId(
                triggerNode["requiredCompletedTutorialId"]
                    ? triggerNode["requiredCompletedTutorialId"]
                          .as<std::string>()
                    : std::string());

            if (triggerNode["talkTexts"] &&
                triggerNode["talkTexts"].IsSequence()) {
                for (const YAML::Node& textNode :
                     triggerNode["talkTexts"]) {
                    trigger->AddTalkTexts(
                        textNode.as<std::string>());
                }
            }
            if (trigger->GetTalkTexts().empty()) {
                trigger->AddTalkTexts("");
            }

            ApplyTalkPageAdvanceConditions(
                trigger,
                triggerNode);

            if (triggerNode["talkStageClearConditions"] &&
                triggerNode["talkStageClearConditions"].IsSequence()) {
                for (const YAML::Node& conditionNode :
                     triggerNode["talkStageClearConditions"]) {
                    if (!conditionNode.IsMap() ||
                        !conditionNode["talkIndex"] ||
                        !conditionNode["stage"]) {
                        continue;
                    }

                    const int talkIndex =
                        conditionNode["talkIndex"].as<int>();
                    const int stageNum =
                        conditionNode["stage"].as<int>();
                    if (talkIndex < 0 || stageNum < 0) {
                        continue;
                    }

                    trigger->SetTalkStageClearCondition(
                        static_cast<std::size_t>(talkIndex),
                        stageNum);
                }
            }

            if (triggerNode["talkCameraFocus"] &&
                triggerNode["talkCameraFocus"].IsSequence()) {
                for (const YAML::Node& focusNode :
                     triggerNode["talkCameraFocus"]) {
                    if (!focusNode.IsMap() ||
                        !focusNode["talkIndex"] ||
                        !focusNode["sequence"] ||
                        !focusNode["index"]) {
                        continue;
                    }

                    const int talkIndex =
                        focusNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    trigger->SetTalkCameraFocusTarget(
                        static_cast<std::size_t>(talkIndex),
                        focusNode["sequence"].as<std::string>(),
                        focusNode["index"].as<int>());
                }
            }

            if (triggerNode["talkRubies"] &&
                triggerNode["talkRubies"].IsSequence()) {
                for (const YAML::Node& rubyNode :
                     triggerNode["talkRubies"]) {
                    if (!rubyNode.IsMap() ||
                        !rubyNode["talkIndex"] ||
                        !rubyNode["segments"] ||
                        !rubyNode["segments"].IsSequence()) {
                        continue;
                    }

                    const int talkIndex =
                        rubyNode["talkIndex"].as<int>();
                    if (talkIndex < 0) {
                        continue;
                    }

                    std::vector<RubyTextSegment> segments;
                    for (const YAML::Node& segmentNode :
                         rubyNode["segments"]) {
                        if (!segmentNode.IsMap() ||
                            !segmentNode["text"]) {
                            continue;
                        }

                        RubyTextSegment segment;
                        segment.text =
                            segmentNode["text"].as<std::string>();
                        segment.reading =
                            segmentNode["reading"]
                                ? segmentNode["reading"].as<std::string>()
                                : std::string();
                        segment.showsRuby =
                            segmentNode["ruby"]
                                ? segmentNode["ruby"].as<bool>()
                                : !segment.reading.empty();
                        segments.emplace_back(std::move(segment));
                    }
                    trigger->SetTalkRubySegments(
                        static_cast<std::size_t>(talkIndex),
                        std::move(segments));
                }
            }

            const std::vector<std::string>& talkTexts =
                trigger->GetTalkTexts();
            for (std::size_t talkIndex = 0;
                 talkIndex < talkTexts.size();
                 ++talkIndex) {
                if (trigger->HasValidTalkRuby(talkIndex)) {
                    continue;
                }

                std::vector<RubyTextSegment> generatedSegments;
                std::string errorMessage;
                if (JapaneseRubyGenerator::Generate(
                        talkTexts[talkIndex],
                        generatedSegments,
                        errorMessage)) {
                    trigger->SetTalkRubySegments(
                        talkIndex,
                        std::move(generatedSegments));
                }
            }
}

void ApplyTutorialTriggerLegacyScale(
    TutorialTrigger& configuredTrigger,
    const YAML::Node& triggerNode)
{
    TutorialTrigger* trigger = &configuredTrigger;
            if ((!triggerNode["scale"] ||
                 !triggerNode["scale"].IsSequence()) &&
                triggerNode["radius"]) {
                const float legacyRadius =
                    std::max(
                        0.01f,
                        triggerNode["radius"].as<float>());
                trigger->SetScale(
                    glm::vec3(legacyRadius));
            }
}

