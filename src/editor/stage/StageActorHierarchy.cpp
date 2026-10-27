#include "editor/stage/StageActorHierarchy.h"

#include "actor/Actor.h"
#include "actor/Planet.h"
#include "editor/stage/StageActorQuery.h"
#include "utils/StringUtils.h"
#include "world/Stage.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {
StageHierarchyCategory CategoryFor(StageActorType type)
{
    switch (type) {
    case StageActorType::Enemy:
        return StageHierarchyCategory::Enemy;
    case StageActorType::Platform:
        return StageHierarchyCategory::Platform;
    case StageActorType::Key:
    case StageActorType::Crystal:
    case StageActorType::JewelItem:
    case StageActorType::Star:
        return StageHierarchyCategory::Item;
    case StageActorType::NPC:
        return StageHierarchyCategory::NPC;
    case StageActorType::Boat:
    case StageActorType::BoatParts:
    case StageActorType::BoatArrivalPoint:
        return StageHierarchyCategory::Boat;
    case StageActorType::CameraZone:
        return StageHierarchyCategory::CameraZone;
    case StageActorType::TutorialTrigger:
        return StageHierarchyCategory::TutorialTrigger;
    case StageActorType::FallRespawnPoint:
    case StageActorType::HazardActor:
        return StageHierarchyCategory::Hazard;
    case StageActorType::StageObject:
        return StageHierarchyCategory::StageObject;
    default:
        return StageHierarchyCategory::Other;
    }
}

const char* CategoryLabel(StageHierarchyCategory category)
{
    switch (category) {
    case StageHierarchyCategory::Enemy:
        return "敵";
    case StageHierarchyCategory::Platform:
        return "足場";
    case StageHierarchyCategory::Item:
        return "アイテム";
    case StageHierarchyCategory::NPC:
        return "NPC";
    case StageHierarchyCategory::Boat:
        return "ボート・パーツ・到着点";
    case StageHierarchyCategory::CameraZone:
        return "カメラゾーン";
    case StageHierarchyCategory::TutorialTrigger:
        return "チュートリアルトリガー";
    case StageHierarchyCategory::Hazard:
        return "落下判定・危険アクター";
    case StageHierarchyCategory::StageObject:
        return "汎用モデル";
    default:
        return "その他";
    }
}

std::string SearchKey(std::string_view text)
{
    auto key = StringUtils::ToLower(std::string(text));
    std::erase_if(key, [](unsigned char value) { return std::isspace(value) != 0; });
    return key;
}
} // namespace

std::vector<StageHierarchyPlanetGroup> StageActorHierarchy::Collect(Stage* stage)
{
    std::vector<StageHierarchyPlanetGroup> hierarchy;
    if (!stage) {
        return hierarchy;
    }

    std::unordered_map<const Planet*, std::size_t> planetGroups;
    const auto& planets = stage->GetPlanets();
    for (std::size_t index = 0; index < planets.size(); ++index) {
        if (!planets[index]) {
            continue;
        }
        StageHierarchyPlanetGroup group;
        group.planetIndex = static_cast<int>(index);
        group.label = "惑星 " + std::to_string(index);
        group.planet = {planets[index], {StageActorType::Planet, static_cast<int>(index), "planets", group.label}};
        planetGroups.emplace(planets[index], hierarchy.size());
        hierarchy.emplace_back(std::move(group));
    }
    StageHierarchyPlanetGroup unassigned;
    unassigned.label = "未所属";

    std::unordered_set<std::string> collected;
    for (const auto& instance : StageActorQuery::CollectAllActorInstances(stage)) {
        if (!instance.actor || instance.ref.type == StageActorType::Planet ||
            !collected.insert(StageActorQuery::MakeKey(instance.ref)).second) {
            continue;
        }
        // 登録一覧が更新途中でも、現在の所属Planetで分類する。表示のために所属情報は書き換えない。
        const auto found = planetGroups.find(instance.actor->GetCurrentPlanet());
        auto& planet = found == planetGroups.end() ? unassigned : hierarchy[found->second];
        const auto category = CategoryFor(instance.ref.type);
        auto group = std::find_if(planet.groups.begin(), planet.groups.end(),
                                  [category](const auto& value) { return value.category == category; });
        if (group == planet.groups.end()) {
            planet.groups.push_back({category, CategoryLabel(category), {}, 0});
            group = std::prev(planet.groups.end());
        }
        group->actors.push_back(instance);
        ++group->actorCount;
        ++planet.actorCount;
    }
    if (unassigned.actorCount > 0) {
        hierarchy.push_back(std::move(unassigned));
    }
    for (auto& planet : hierarchy) {
        std::stable_sort(planet.groups.begin(), planet.groups.end(),
                         [](const auto& left, const auto& right) { return left.category < right.category; });
    }
    return hierarchy;
}

std::vector<StageHierarchyPlanetGroup>
StageActorHierarchy::Filter(const std::vector<StageHierarchyPlanetGroup>& hierarchy, std::string_view query)
{
    const auto key = SearchKey(query);
    if (key.empty()) {
        return hierarchy;
    }
    std::vector<StageHierarchyPlanetGroup> filtered;
    for (const auto& planet : hierarchy) {
        if (SearchKey(planet.label).find(key) != std::string::npos) {
            filtered.push_back(planet);
            continue;
        }
        auto matchingPlanet = planet;
        matchingPlanet.groups.clear();
        for (const auto& group : planet.groups) {
            if (SearchKey(group.label).find(key) != std::string::npos) {
                matchingPlanet.groups.push_back(group);
                continue;
            }
            auto matchingGroup = group;
            matchingGroup.actors.clear();
            for (const auto& actor : group.actors) {
                if (SearchKey(actor.ref.label).find(key) != std::string::npos ||
                    SearchKey(StageActorQuery::GetTypeLabel(actor.ref)).find(key) != std::string::npos) {
                    matchingGroup.actors.push_back(actor);
                }
            }
            if (!matchingGroup.actors.empty()) {
                matchingPlanet.groups.push_back(std::move(matchingGroup));
            }
        }
        if (!matchingPlanet.groups.empty()) {
            filtered.push_back(std::move(matchingPlanet));
        }
    }
    return filtered;
}
