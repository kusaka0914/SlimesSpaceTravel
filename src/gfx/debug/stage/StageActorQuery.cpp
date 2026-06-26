#include "gfx/debug/stage/StageActorQuery.h"

#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatArrivalPoint.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/MovingPlatform.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"

#include <string>
#include <vector>

namespace {

void AddInstance(std::vector<StageActorInstance>& instances, Actor* actor, StageActorType type, int yamlIndex,
                 const std::string& sequenceName, const std::string& label)
{
    if (!actor || yamlIndex < 0) {
        return;
    }

    StageActorRef ref;
    ref.type = type;
    ref.yamlIndex = yamlIndex;
    ref.sequenceName = sequenceName;
    ref.label = label;

    StageActorInstance instance;
    instance.actor = actor;
    instance.ref = ref;

    instances.emplace_back(instance);
}

std::string MakeIndexedLabel(const char* typeLabel, int yamlIndex)
{
    return std::string(typeLabel) + " " + std::to_string(yamlIndex);
}

} // namespace

const std::vector<StageActorTypeInfo>& StageActorQuery::GetTypeInfos()
{
    static const std::vector<StageActorTypeInfo> typeInfos = {
        {StageActorType::Enemy, "enemies", "敵"},
        {StageActorType::Platform, "platforms", "足場"},
        {StageActorType::MovingPlatform, "movingPlatforms", "動く足場"},
        {StageActorType::Key, "keys", "キー"},
        {StageActorType::Boat, "boats", "ボート"},
        {StageActorType::BoatParts, "boatParts", "ボートパーツ"},
        {StageActorType::Crystal, "crystals", "クリスタル"},
        {StageActorType::NPC, "NPCs", "NPC"},
        {StageActorType::Star, "star", "星"},
        {StageActorType::BoatArrivalPoint, "boatArrivalPoints", "ボート到着点"},
    };

    return typeInfos;
}

std::vector<StageActorInstance> StageActorQuery::CollectAllActorInstances(Stage* stage)
{
    std::vector<StageActorInstance> instances;

    if (!stage) {
        return instances;
    }

    const std::vector<Planet*> planets = stage->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            const int yamlIndex = enemy ? enemy->GetStageYamlIndex() : -1;
            AddInstance(instances, enemy, StageActorType::Enemy, yamlIndex, "enemies",
                        MakeIndexedLabel("敵", yamlIndex));
        }

        for (Platform* platform : planet->GetPlatforms()) {
            const int yamlIndex = platform ? platform->GetStageYamlIndex() : -1;
            AddInstance(instances, platform, StageActorType::Platform, yamlIndex, "platforms",
                        MakeIndexedLabel("足場", yamlIndex));
        }

        for (MovingPlatform* platform : planet->GetMovingPlatforms()) {
            const int yamlIndex = platform ? platform->GetStageYamlIndex() : -1;
            AddInstance(instances, platform, StageActorType::MovingPlatform, yamlIndex, "movingPlatforms",
                        MakeIndexedLabel("動く足場", yamlIndex));
        }

        if (Key* key = planet->GetKey()) {
            const int yamlIndex = key->GetStageYamlIndex();
            AddInstance(instances, key, StageActorType::Key, yamlIndex, "keys", MakeIndexedLabel("キー", yamlIndex));
        }

        for (Boat* boat : planet->GetBoats()) {
            const int yamlIndex = boat ? boat->GetStageYamlIndex() : -1;
            AddInstance(instances, boat, StageActorType::Boat, yamlIndex, "boats",
                        MakeIndexedLabel("ボート", yamlIndex));
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            const int yamlIndex = part ? part->GetStageYamlIndex() : -1;
            AddInstance(instances, part, StageActorType::BoatParts, yamlIndex, "boatParts",
                        MakeIndexedLabel("ボートパーツ", yamlIndex));
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            const int yamlIndex = crystal ? crystal->GetStageYamlIndex() : -1;
            AddInstance(instances, crystal, StageActorType::Crystal, yamlIndex, "crystals",
                        MakeIndexedLabel("クリスタル", yamlIndex));
        }

        for (NPC* npc : planet->GetNPCs()) {
            const int yamlIndex = npc ? npc->GetStageYamlIndex() : -1;
            AddInstance(instances, npc, StageActorType::NPC, yamlIndex, "NPCs", MakeIndexedLabel("NPC", yamlIndex));
        }

        if (Star* star = planet->GetStar()) {
            const int yamlIndex = star->GetStageYamlIndex();
            AddInstance(instances, star, StageActorType::Star, yamlIndex, "star", MakeIndexedLabel("星", yamlIndex));
        }

        for (BoatArrivalPoint* point : planet->GetBoatArrivalPoints()) {
            const int yamlIndex = point ? point->GetStageYamlIndex() : -1;
            AddInstance(instances, point, StageActorType::BoatArrivalPoint, yamlIndex, "boatArrivalPoints",
                        MakeIndexedLabel("ボート到着点", yamlIndex));
        }
    }

    return instances;
}

std::vector<StageActorRef> StageActorQuery::CollectAllTargets(Stage* stage)
{
    std::vector<StageActorRef> targets;

    for (const StageActorInstance& instance : CollectAllActorInstances(stage)) {
        targets.emplace_back(instance.ref);
    }

    return targets;
}

std::optional<StageActorRef> StageActorQuery::FindTargetForActor(Stage* stage, Actor* actor)
{
    if (!stage || !actor) {
        return std::nullopt;
    }

    for (const StageActorInstance& instance : CollectAllActorInstances(stage)) {
        if (instance.actor == actor) {
            return instance.ref;
        }
    }

    return std::nullopt;
}

Actor* StageActorQuery::FindActorByRef(Stage* stage, const StageActorRef& target)
{
    if (!stage) {
        return nullptr;
    }

    const std::string targetKey = MakeKey(target);

    for (const StageActorInstance& instance : CollectAllActorInstances(stage)) {
        if (MakeKey(instance.ref) == targetKey) {
            return instance.actor;
        }
    }

    return nullptr;
}

std::string StageActorQuery::MakeKey(const StageActorRef& target)
{
    return target.sequenceName + ":" + std::to_string(target.yamlIndex);
}

std::string StageActorQuery::GetSequenceName(StageActorType type)
{
    for (const StageActorTypeInfo& info : GetTypeInfos()) {
        if (info.type == type) {
            return info.sequenceName;
        }
    }

    return "";
}

const char* StageActorQuery::GetTypeLabel(StageActorType type)
{
    for (const StageActorTypeInfo& info : GetTypeInfos()) {
        if (info.type == type) {
            return info.displayName;
        }
    }

    return "不明";
}