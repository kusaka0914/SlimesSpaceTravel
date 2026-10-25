#include "system/actor_loader/NpcStageConfigApplicator.h"

#include "actor/NPC.h"
#include "system/actor_loader/NpcTalkStageConfigApplicator.h"
#include "system/text/JapaneseRubyGenerator.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace {

void ApplyTalkOpeningAfterPages(NPC* npc, const YAML::Node& actorNode)
{
    if (!npc || !actorNode["talkOpeningAfterPages"] || !actorNode["talkOpeningAfterPages"].IsSequence()) {
        return;
    }

    for (const YAML::Node& pageNode : actorNode["talkOpeningAfterPages"]) {
        if (!pageNode.IsMap() || !pageNode["talkIndex"]) {
            continue;
        }

        const int talkIndex = pageNode["talkIndex"].as<int>();
        if (talkIndex >= 0) {
            npc->SetTalkStartsOpeningAfterPage(static_cast<std::size_t>(talkIndex), true);
        }
    }
}

void ApplyTalkEndingAfterPages(NPC* npc, const YAML::Node& actorNode)
{
    if (!npc || !actorNode["talkEndingAfterPages"] || !actorNode["talkEndingAfterPages"].IsSequence()) {
        return;
    }

    for (const YAML::Node& pageNode : actorNode["talkEndingAfterPages"]) {
        if (!pageNode.IsMap() || !pageNode["talkIndex"]) {
            continue;
        }
        const int talkIndex = pageNode["talkIndex"].as<int>();
        if (talkIndex >= 0) {
            npc->SetTalkStartsEndingAfterPage(static_cast<std::size_t>(talkIndex), true);
        }
    }
}

} // namespace

void ApplyNpcStageConfig(NPC& npc, const YAML::Node& node)
{
    const float facingYaw = node["facingYaw"] ? node["facingYaw"].as<float>() : 0.0f;
    npc.SetFacingYaw(facingYaw);

    const float radius = node["radius"] ? node["radius"].as<float>() : 0.75f;
    npc.SetRadius(radius);

    const std::string name = node["name"] ? node["name"].as<std::string>() : "";
    npc.SetName(name);

    npc.SetForcesTalkOnArrival(node["forceTalkOnArrival"] && node["forceTalkOnArrival"].as<bool>());

    if (node["proximityMessage"] && node["proximityMessage"].IsMap()) {
        const YAML::Node messageNode = node["proximityMessage"];
        const std::string mode = messageNode["mode"] ? messageNode["mode"].as<std::string>() : "disabled";
        if (mode == "afterTalk") {
            npc.SetProximityMessageMode(NPCProximityMessageMode::AfterTalk);
        } else if (mode == "always") {
            npc.SetProximityMessageMode(NPCProximityMessageMode::Always);
        } else {
            npc.SetProximityMessageMode(NPCProximityMessageMode::Disabled);
        }

        npc.SetProximityMessageRange(messageNode["range"] ? messageNode["range"].as<float>() : 3.0f);
        npc.SetProximityMessageHeight(messageNode["height"] ? messageNode["height"].as<float>() : 1.8f);
        npc.SetProximityMessageScale(messageNode["scale"] ? messageNode["scale"].as<float>() : 1.0f);
    }

    if (node["talkTexts"] && node["talkTexts"].IsSequence()) {
        for (const YAML::Node& talkTextNode : node["talkTexts"]) {
            npc.AddTalkTexts(talkTextNode.as<std::string>());
        }
    }

    if (npc.GetTalkTexts().empty() && npc.GetProximityMessageMode() != NPCProximityMessageMode::Disabled) {
        npc.AddTalkTexts("");
    }

    ApplyNpcTalkAdvanceConditions(npc, node);
    ApplyTalkOpeningAfterPages(&npc, node);
    ApplyTalkEndingAfterPages(&npc, node);

    if (node["proximityMessage"] && node["proximityMessage"].IsMap()) {
        const YAML::Node messageNode = node["proximityMessage"];
        bool loadedVariants = false;
        if (messageNode["variants"] && messageNode["variants"].IsSequence()) {
            for (const YAML::Node& variantNode : messageNode["variants"]) {
                if (!variantNode.IsMap() || !variantNode["talkIndex"] || !variantNode["text"]) {
                    continue;
                }

                const int talkIndex = variantNode["talkIndex"].as<int>();
                if (talkIndex < 0) {
                    continue;
                }
                npc.SetTalkProximityMessageText(static_cast<std::size_t>(talkIndex),
                                                variantNode["text"].as<std::string>());
                loadedVariants = true;
            }
        }

        if (!loadedVariants && messageNode["text"]) {
            const std::string legacyText = messageNode["text"].as<std::string>();
            for (std::size_t talkIndex = 0; talkIndex < npc.GetTalkTexts().size(); ++talkIndex) {
                npc.SetTalkProximityMessageText(talkIndex, legacyText);
            }
        }

        if (messageNode["rubies"] && messageNode["rubies"].IsSequence()) {
            for (const YAML::Node& rubyNode : messageNode["rubies"]) {
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

                npc.SetTalkProximityMessageRubySegments(static_cast<std::size_t>(talkIndex), std::move(segments));
            }
        }

        for (std::size_t talkIndex = 0; talkIndex < npc.GetTalkTexts().size(); ++talkIndex) {
            const std::string& proximityText = npc.GetTalkProximityMessageText(talkIndex);
            if (proximityText.empty() || npc.HasValidTalkProximityMessageRuby(talkIndex)) {
                continue;
            }

            std::vector<RubyTextSegment> generatedSegments;
            std::string errorMessage;
            if (JapaneseRubyGenerator::Generate(proximityText, generatedSegments, errorMessage)) {
                npc.SetTalkProximityMessageRubySegments(talkIndex, std::move(generatedSegments));
            }
        }
    }

    ApplyNpcTalkPageConfig(npc, node);
}
