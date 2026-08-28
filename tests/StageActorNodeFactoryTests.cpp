#include "TestSupport.h"

#include "gfx/debug/stage/StageActorNodeFactory.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

StageActorNodeFactory CreateNodeFactory()
{
    return StageActorNodeFactory(
        [](int planetIndex, float height) {
            return 10.0f * static_cast<float>(planetIndex) + height;
        });
}

void CreatePlatformStoresPlacementDefaultsAndVisualSettings()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreatePlatform(
        2, "models/platform.obj", glm::vec3(1.0f, 2.0f, 3.0f));

    ExpectEqual(2, node["currentPlanetNum"].as<int>(), "planet index");
    ExpectEqual(
        std::string("models/platform.obj"),
        node["modelPath"].as<std::string>(),
        "model path");
    ExpectNear(1.0f, node["height"].as<float>(), 0.0001f, "height");
    ExpectNear(1.0f, node["scale"][0].as<float>(), 0.0001f, "scale x");
    ExpectNear(2.0f, node["scale"][1].as<float>(), 0.0001f, "scale y");
    ExpectNear(3.0f, node["scale"][2].as<float>(), 0.0001f, "scale z");
}

void CreateRideMovingPlatformAddsMovementComponentDefaults()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateRideMovingPlatform(
        1, "platform.obj", glm::vec3(1.0f));
    const YAML::Node movement = node["components"]["movement"];

    ExpectTrue(movement["moveOnPlayer"].as<bool>(), "moveOnPlayer");
    ExpectNear(
        3.0f, movement["moveDuration"].as<float>(), 0.0001f,
        "move duration");
    ExpectNear(
        1.0f, movement["returnDelay"].as<float>(), 0.0001f,
        "return delay");
    ExpectNear(
        5.0f, movement["moveOffset"][1].as<float>(), 0.0001f,
        "vertical move offset");
}

void CreatePlanetUsesIndexForInitialCenterAndStageNumber()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node =
        nodeFactory.CreatePlanet(3, "planet.obj");

    ExpectNear(96.0f, node["center"][0].as<float>(), 0.0001f, "center x");
    ExpectEqual(3, node["stageNum"].as<int>(), "stage number");
    ExpectEqual(
        std::string("Sphere"), node["shape"].as<std::string>(),
        "planet shape");
    ExpectTrue(
        node["canAttractNearbyPlayer"].as<bool>(),
        "nearby player attraction");
}

void CreateEnemyUsesTypeAndCalculatedSurfaceDistance()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node bossNode = nodeFactory.CreateEnemy("boss", 2);

    ExpectEqual(
        std::string("boss"), bossNode["type"].as<std::string>(),
        "enemy type");
    ExpectEqual(
        std::string("新しいボス敵"),
        bossNode["editorName"].as<std::string>(),
        "boss editor name");
    ExpectNear(
        21.0f, bossNode["pos"][0].as<float>(), 0.0001f,
        "surface distance");
}

void CreateNPCClampsRadiusAndScaleAndProvidesEmptyDialogue()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateNPC(
        "npc.obj", 1, "案内役", {}, -2.0f, 0.0f);

    ExpectNear(0.1f, node["radius"].as<float>(), 0.0001f, "radius");
    ExpectNear(0.01f, node["scale"][0].as<float>(), 0.0001f, "scale");
    ExpectEqual(
        std::size_t{1}, node["talkTexts"].size(),
        "dialogue count");
    ExpectEqual(
        std::string(""), node["talkTexts"][0].as<std::string>(),
        "default dialogue");
}

void CreateNPCPreservesDialogueOrder()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateNPC(
        "npc.obj", 0, "案内役", {"最初", "次"}, 2.0f, 1.5f);

    ExpectEqual(
        std::size_t{2}, node["talkTexts"].size(),
        "dialogue count");
    ExpectEqual(
        std::string("最初"), node["talkTexts"][0].as<std::string>(),
        "first dialogue");
    ExpectEqual(
        std::string("次"), node["talkTexts"][1].as<std::string>(),
        "second dialogue");
}

void CreateTutorialTriggerClampsEachScaleAxis()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateTutorialTrigger(
        1, "trigger.obj", {}, glm::vec3(-1.0f, 2.0f, 0.0f));

    ExpectNear(0.01f, node["scale"][0].as<float>(), 0.0001f, "scale x");
    ExpectNear(2.0f, node["scale"][1].as<float>(), 0.0001f, "scale y");
    ExpectNear(0.01f, node["scale"][2].as<float>(), 0.0001f, "scale z");
    ExpectEqual(
        std::size_t{1}, node["talkTexts"].size(),
        "default tutorial text count");
}

void CreateCollectiblesStoreTypePlanetAndActivationDefaults()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node crystalNode =
        nodeFactory.CreateCrystal("blue", 4);
    const YAML::Node boatPartsNode =
        nodeFactory.CreateBoatParts("engine", 2);
    const YAML::Node starNode = nodeFactory.CreateStar(3);

    ExpectEqual(
        std::string("blue"), crystalNode["type"].as<std::string>(),
        "crystal type");
    ExpectEqual(4, crystalNode["currentPlanetNum"].as<int>(), "crystal planet");
    ExpectEqual(
        std::string("engine"), boatPartsNode["type"].as<std::string>(),
        "boat parts type");
    ExpectEqual(3, starNode["currentPlanetNum"].as<int>(), "star planet");
    ExpectFalse(starNode["isActive"].as<bool>(), "star initial activation");
}

void CreateBoatStoresRouteAndCalculatedSurfaceDistance()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateBoat(2, 5, 7);

    ExpectEqual(2, node["startPlanet"].as<int>(), "start planet");
    ExpectEqual(5, node["destPlanet"].as<int>(), "destination planet");
    ExpectEqual(7, node["destStage"].as<int>(), "destination stage");
    ExpectNear(
        21.0f, node["pos"][0].as<float>(), 0.0001f,
        "boat surface distance");
}

void CreateBoatArrivalPointClampsScale()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateBoatArrivalPoint(
        2, "arrival.obj", glm::vec3(0.0f, -1.0f, 3.0f));

    ExpectNear(0.01f, node["scale"][0].as<float>(), 0.0001f, "scale x");
    ExpectNear(0.01f, node["scale"][1].as<float>(), 0.0001f, "scale y");
    ExpectNear(3.0f, node["scale"][2].as<float>(), 0.0001f, "scale z");
}

void CreateJewelItemOmitsEmptyTextureAndClampsScale()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateJewelItem(
        1, "jewel.obj", "", glm::vec3(-1.0f, 2.0f, 3.0f));

    ExpectFalse(static_cast<bool>(node["textureOverride"]), "texture override");
    ExpectNear(0.01f, node["scale"][0].as<float>(), 0.0001f, "scale x");
    ExpectNear(2.0f, node["scale"][1].as<float>(), 0.0001f, "scale y");
}

void CreateHazardActorClampsUnsafeParameters()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node = nodeFactory.CreateHazardActor(
        1,
        "hazard.obj",
        "hazard.png",
        glm::vec3(0.0f),
        -1.0f,
        -5.0f,
        -2.0f);

    ExpectEqual(
        std::string("hazard.png"),
        node["textureOverride"].as<std::string>(),
        "texture override");
    ExpectNear(
        0.01f, node["triggerRadius"].as<float>(), 0.0001f,
        "trigger radius");
    ExpectNear(0.0f, node["damage"].as<float>(), 0.0001f, "damage");
    ExpectNear(
        0.0f, node["damageIntervalSeconds"].as<float>(), 0.0001f,
        "damage interval");
}

void CreateStageObjectStoresCollisionAndCalculatedSurfaceDistance()
{
    const StageActorNodeFactory nodeFactory = CreateNodeFactory();
    const YAML::Node node =
        nodeFactory.CreateStageObject(3, "tree.obj", true);

    ExpectTrue(node["collision"].as<bool>(), "collision enabled");
    ExpectNear(
        31.0f, node["pos"][0].as<float>(), 0.0001f,
        "stage object surface distance");
    ExpectEqual(
        std::string("tree.obj"), node["modelPath"].as<std::string>(),
        "model path");
}

}

void RegisterStageActorNodeFactoryTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "StageActorNodeFactory.CreatePlatformStoresPlacementDefaultsAndVisualSettings",
        CreatePlatformStoresPlacementDefaultsAndVisualSettings);
    tests.emplace_back(
        "StageActorNodeFactory.CreateRideMovingPlatformAddsMovementComponentDefaults",
        CreateRideMovingPlatformAddsMovementComponentDefaults);
    tests.emplace_back(
        "StageActorNodeFactory.CreatePlanetUsesIndexForInitialCenterAndStageNumber",
        CreatePlanetUsesIndexForInitialCenterAndStageNumber);
    tests.emplace_back(
        "StageActorNodeFactory.CreateEnemyUsesTypeAndCalculatedSurfaceDistance",
        CreateEnemyUsesTypeAndCalculatedSurfaceDistance);
    tests.emplace_back(
        "StageActorNodeFactory.CreateNPCClampsRadiusAndScaleAndProvidesEmptyDialogue",
        CreateNPCClampsRadiusAndScaleAndProvidesEmptyDialogue);
    tests.emplace_back(
        "StageActorNodeFactory.CreateNPCPreservesDialogueOrder",
        CreateNPCPreservesDialogueOrder);
    tests.emplace_back(
        "StageActorNodeFactory.CreateTutorialTriggerClampsEachScaleAxis",
        CreateTutorialTriggerClampsEachScaleAxis);
    tests.emplace_back(
        "StageActorNodeFactory.CreateCollectiblesStoreTypePlanetAndActivationDefaults",
        CreateCollectiblesStoreTypePlanetAndActivationDefaults);
    tests.emplace_back(
        "StageActorNodeFactory.CreateBoatStoresRouteAndCalculatedSurfaceDistance",
        CreateBoatStoresRouteAndCalculatedSurfaceDistance);
    tests.emplace_back(
        "StageActorNodeFactory.CreateBoatArrivalPointClampsScale",
        CreateBoatArrivalPointClampsScale);
    tests.emplace_back(
        "StageActorNodeFactory.CreateJewelItemOmitsEmptyTextureAndClampsScale",
        CreateJewelItemOmitsEmptyTextureAndClampsScale);
    tests.emplace_back(
        "StageActorNodeFactory.CreateHazardActorClampsUnsafeParameters",
        CreateHazardActorClampsUnsafeParameters);
    tests.emplace_back(
        "StageActorNodeFactory.CreateStageObjectStoresCollisionAndCalculatedSurfaceDistance",
        CreateStageObjectStoresCollisionAndCalculatedSurfaceDistance);
}
