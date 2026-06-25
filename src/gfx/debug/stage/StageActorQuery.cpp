#include "gfx/debug/stage/StageActorQuery.h"

#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Star.h"

#include <string>
#include <vector>

namespace {

void AddTarget(std::vector<StageActorRef>& targets, StageActorType type, int yamlIndex, const std::string& sequenceName,
               const std::string& label)
{
    if (yamlIndex < 0) {
        return;
    }

    StageActorRef target;
    target.type = type;
    target.yamlIndex = yamlIndex;
    target.sequenceName = sequenceName;
    target.label = label;

    targets.emplace_back(target);
}

} // namespace

std::vector<StageActorRef> StageActorQuery::CollectAllTargets(Stage* stage)
{
    std::vector<StageActorRef> targets;

    if (!stage) {
        return targets;
    }

    const std::vector<Planet*> planets = stage->GetPlanets();

    for (Planet* planet : planets) {
        if (!planet) {
            continue;
        }

        for (Enemy* enemy : planet->GetEnemies()) {
            if (!enemy) {
                continue;
            }

            AddTarget(targets, StageActorType::Enemy, enemy->GetStageYamlIndex(), "enemies",
                      "敵 " + std::to_string(enemy->GetStageYamlIndex()));
        }

        for (Platform* platform : planet->GetPlatforms()) {
            if (!platform) {
                continue;
            }

            AddTarget(targets, StageActorType::Platform, platform->GetStageYamlIndex(), "platforms",
                      "足場 " + std::to_string(platform->GetStageYamlIndex()));
        }

        if (Key* key = planet->GetKey()) {
            AddTarget(targets, StageActorType::Key, key->GetStageYamlIndex(), "keys",
                      "キー " + std::to_string(key->GetStageYamlIndex()));
        }

        for (Boat* boat : planet->GetBoats()) {
            if (!boat) {
                continue;
            }

            AddTarget(targets, StageActorType::Boat, boat->GetStageYamlIndex(), "boats",
                      "ボート " + std::to_string(boat->GetStageYamlIndex()));
        }

        for (BoatParts* part : planet->GetBoatParts()) {
            if (!part) {
                continue;
            }

            AddTarget(targets, StageActorType::BoatParts, part->GetStageYamlIndex(), "boatParts",
                      "ボートパーツ " + std::to_string(part->GetStageYamlIndex()));
        }

        for (Crystal* crystal : planet->GetCrystals()) {
            if (!crystal) {
                continue;
            }

            AddTarget(targets, StageActorType::Crystal, crystal->GetStageYamlIndex(), "crystals",
                      "クリスタル " + std::to_string(crystal->GetStageYamlIndex()));
        }

        for (NPC* npc : planet->GetNPCs()) {
            if (!npc) {
                continue;
            }

            AddTarget(targets, StageActorType::NPC, npc->GetStageYamlIndex(), "NPCs",
                      "NPC " + std::to_string(npc->GetStageYamlIndex()));
        }

        if (Star* star = planet->GetStar()) {
            AddTarget(targets, StageActorType::Star, star->GetStageYamlIndex(), "star",
                      "星 " + std::to_string(star->GetStageYamlIndex()));
        }
    }

    return targets;
}

std::optional<StageActorRef> StageActorQuery::FindTargetForActor(Stage* stage, Actor* actor)
{
    if (!stage || !actor) {
        return std::nullopt;
    }

    const std::vector<StageActorRef> targets = CollectAllTargets(stage);

    for (const StageActorRef& target : targets) {
        if (target.yamlIndex != actor->GetStageYamlIndex()) {
            continue;
        }

        if (target.sequenceName == "enemies" && dynamic_cast<Enemy*>(actor)) {
            return target;
        }

        if (target.sequenceName == "platforms" && dynamic_cast<Platform*>(actor)) {
            return target;
        }

        if (target.sequenceName == "keys" && dynamic_cast<Key*>(actor)) {
            return target;
        }

        if (target.sequenceName == "boats" && dynamic_cast<Boat*>(actor)) {
            return target;
        }

        if (target.sequenceName == "boatParts" && dynamic_cast<BoatParts*>(actor)) {
            return target;
        }

        if (target.sequenceName == "crystals" && dynamic_cast<Crystal*>(actor)) {
            return target;
        }

        if (target.sequenceName == "NPCs" && dynamic_cast<NPC*>(actor)) {
            return target;
        }

        if (target.sequenceName == "star" && dynamic_cast<Star*>(actor)) {
            return target;
        }
    }

    return std::nullopt;
}

std::string StageActorQuery::MakeKey(const StageActorRef& target)
{
    return target.sequenceName + ":" + std::to_string(target.yamlIndex);
}

std::string StageActorQuery::GetSequenceName(StageActorType type)
{
    switch (type) {
    case StageActorType::Enemy:
        return "enemies";
    case StageActorType::Platform:
        return "platforms";
    case StageActorType::Crystal:
        return "crystals";
    case StageActorType::NPC:
        return "NPCs";
    case StageActorType::BoatParts:
        return "boatParts";
    case StageActorType::Boat:
        return "boats";
    case StageActorType::Key:
        return "keys";
    case StageActorType::Star:
        return "star";
    default:
        return "";
    }
}

const char* StageActorQuery::GetTypeLabel(StageActorType type)
{
    switch (type) {
    case StageActorType::Enemy:
        return "敵";
    case StageActorType::Platform:
        return "足場";
    case StageActorType::Crystal:
        return "クリスタル";
    case StageActorType::NPC:
        return "NPC";
    case StageActorType::BoatParts:
        return "ボートパーツ";
    case StageActorType::Boat:
        return "ボート";
    case StageActorType::Key:
        return "キー";
    case StageActorType::Star:
        return "星";
    default:
        return "不明";
    }
}