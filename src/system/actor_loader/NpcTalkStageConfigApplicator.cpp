#include "system/actor_loader/NpcTalkStageConfigApplicator.h"

#include "actor/NPC.h"
#include "system/text/JapaneseRubyGenerator.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

void ApplyNpcTalkAdvanceConditions(NPC& npc, const YAML::Node& node)
{
    if (!node["talkAdvanceConditions"] || !node["talkAdvanceConditions"].IsSequence()) {
        return;
    }

    for (const YAML::Node& conditionNode : node["talkAdvanceConditions"]) {
        if (!conditionNode.IsMap() || !conditionNode["talkIndex"] || !conditionNode["condition"]) {
            continue;
        }

        const int talkIndex = conditionNode["talkIndex"].as<int>();
        if (talkIndex < 0) {
            continue;
        }

        npc.SetTalkAdvanceCondition(static_cast<std::size_t>(talkIndex),
                                    ParseTalkPageAdvanceConditionId(conditionNode["condition"].as<std::string>()));
    }
}

void ApplyNpcTalkPageConfig(NPC& npc, const YAML::Node& node)
{
    if (node["talkStageClearConditions"] && node["talkStageClearConditions"].IsSequence()) {
        for (const YAML::Node& conditionNode : node["talkStageClearConditions"]) {
            if (!conditionNode.IsMap() || !conditionNode["talkIndex"] || !conditionNode["stage"]) {
                continue;
            }

            const int talkIndex = conditionNode["talkIndex"].as<int>();
            const int stageNum = conditionNode["stage"].as<int>();
            if (talkIndex < 0 || stageNum < 0) {
                continue;
            }
            npc.SetTalkStageClearCondition(static_cast<std::size_t>(talkIndex), stageNum);
        }
    }

    if (node["talkCameraFocus"] && node["talkCameraFocus"].IsSequence()) {
        for (const YAML::Node& focusNode : node["talkCameraFocus"]) {
            if (!focusNode.IsMap() || !focusNode["talkIndex"] || !focusNode["sequence"] || !focusNode["index"]) {
                continue;
            }

            const int talkIndex = focusNode["talkIndex"].as<int>();
            if (talkIndex < 0) {
                continue;
            }

            npc.SetTalkCameraFocusTarget(static_cast<std::size_t>(talkIndex), focusNode["sequence"].as<std::string>(),
                                         focusNode["index"].as<int>());
        }
    }

    if (node["talkRubies"] && node["talkRubies"].IsSequence()) {
        for (const YAML::Node& rubyNode : node["talkRubies"]) {
            if (!rubyNode.IsMap() || !rubyNode["talkIndex"] || !rubyNode["segments"] ||
                !rubyNode["segments"].IsSequence()) {
                continue;
            }

            const int talkIndex = rubyNode["talkIndex"].as<int>();
            if (talkIndex < 0) {
                continue;
            }

            std::vector<RubyTextSegment> segments;
            for (const YAML::Node& segmentNode : rubyNode["segments"]) {
                if (!segmentNode.IsMap() || !segmentNode["text"]) {
                    continue;
                }

                RubyTextSegment segment;
                segment.text = segmentNode["text"].as<std::string>();
                segment.reading = segmentNode["reading"] ? segmentNode["reading"].as<std::string>() : std::string();
                segment.showsRuby = segmentNode["ruby"] ? segmentNode["ruby"].as<bool>() : !segment.reading.empty();
                segments.emplace_back(std::move(segment));
            }

            npc.SetTalkRubySegments(static_cast<std::size_t>(talkIndex), std::move(segments));
        }
    }

    const std::vector<std::string>& talkTexts = npc.GetTalkTexts();
    for (std::size_t talkIndex = 0; talkIndex < talkTexts.size(); ++talkIndex) {
        if (npc.HasValidTalkRuby(talkIndex)) {
            continue;
        }

        std::vector<RubyTextSegment> generatedSegments;
        std::string errorMessage;
        if (JapaneseRubyGenerator::Generate(talkTexts[talkIndex], generatedSegments, errorMessage)) {
            npc.SetTalkRubySegments(talkIndex, std::move(generatedSegments));
        }
    }
}
