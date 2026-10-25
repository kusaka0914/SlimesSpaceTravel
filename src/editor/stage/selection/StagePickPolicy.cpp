#include "editor/stage/selection/StagePickPolicy.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_set>

int StagePickPolicy::ResolveLayer(const YAML::Node& stage, const StageActorRef& actor, float worldY, float gridSize)
{
    const double rounded = gridSize > 0.0f ? std::round(worldY / gridSize) : 0.0;
    const int fallback = std::isfinite(rounded) && rounded >= std::numeric_limits<int>::min() &&
                                 rounded <= std::numeric_limits<int>::max()
                             ? static_cast<int>(rounded)
                             : 0;
    try {
        if (!stage.IsMap() || actor.yamlIndex < 0) {
            return fallback;
        }
        const YAML::Node sequence = stage[actor.sequenceName];
        if (!sequence.IsSequence() || static_cast<std::size_t>(actor.yamlIndex) >= sequence.size()) {
            return fallback;
        }
        const YAML::Node node = sequence[actor.yamlIndex];
        if (node.IsMap() && node["ugcGeneratedPlatform"].as<bool>(false)) {
            return node["ugcGridLayer"].as<int>(fallback);
        }
    } catch (const YAML::Exception&) {
        // 選択補助のメタデータが壊れていても、Runtimeに存在するActorは選択可能にする。
    }
    return fallback;
}

std::vector<std::size_t> StagePickPolicy::SelectLayers(const std::vector<StagePickCandidate>& candidates, int editLayer,
                                                       bool includeAllLayers)
{
    int selectedLayer = std::numeric_limits<int>::min();
    bool hasEditLayer = false;
    for (const auto& candidate : candidates) {
        if (candidate.identity) {
            hasEditLayer |= candidate.layer == editLayer;
            selectedLayer = std::max(selectedLayer, candidate.layer);
        }
    }
    if (hasEditLayer) {
        selectedLayer = editLayer;
    }

    std::vector<std::size_t> selected;
    std::unordered_set<const void*> seen;
    for (const auto& candidate : candidates) {
        if (candidate.identity && (includeAllLayers || candidate.layer == selectedLayer) &&
            seen.insert(candidate.identity).second) {
            selected.push_back(candidate.sourceIndex);
        }
    }
    return selected;
}
