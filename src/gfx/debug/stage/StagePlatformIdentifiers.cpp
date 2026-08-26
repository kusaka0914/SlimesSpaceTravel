#include "gfx/debug/stage/StagePlatformIdentifiers.h"

#include <unordered_set>

namespace StagePlatformIdentifiers {

std::string CreateUniqueId(const YAML::Node& stageConfig)
{
    std::unordered_set<std::string> usedIds;
    if (stageConfig && stageConfig.IsMap()) {
        for (const auto& entry : stageConfig) {
            const YAML::Node sequence = entry.second;
            if (!sequence || !sequence.IsSequence()) {
                continue;
            }
            for (const YAML::Node& actorNode : sequence) {
                if (!actorNode || !actorNode.IsMap()) {
                    continue;
                }

                if (actorNode["platformId"] &&
                    actorNode["platformId"].IsScalar()) {
                    usedIds.insert(
                        actorNode["platformId"].as<std::string>());
                }

                const YAML::Node components = actorNode["components"];
                if (!components || !components.IsMap()) {
                    continue;
                }

                const YAML::Node pressureSwitch =
                    components["pressureSwitch"];
                if (!pressureSwitch || !pressureSwitch.IsMap()) {
                    continue;
                }

                const YAML::Node targets = pressureSwitch["targets"];
                if (!targets || !targets.IsSequence()) {
                    continue;
                }
                for (const YAML::Node& target : targets) {
                    if (target && target.IsScalar()) {
                        usedIds.insert(target.as<std::string>());
                    }
                }
            }
        }
    }

    for (int suffix = 1;; ++suffix) {
        const std::string candidate =
            "platform_" + std::to_string(suffix);
        if (!usedIds.contains(candidate)) {
            return candidate;
        }
    }
}

}
