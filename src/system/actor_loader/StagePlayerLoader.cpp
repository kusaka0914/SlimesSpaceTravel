#include "system/actor_loader/StagePlayerLoader.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/MeshLoadSystem.h"
#include "system/actor_loader/ActorPlacementLoader.h"

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

StagePlayerLoader::StagePlayerLoader(
    Game* game,
    const ActorPlacementLoader& placementLoader)
    : mGame(game),
      mPlacementLoader(placementLoader)
{
}

void StagePlayerLoader::LoadPlayers(const YAML::Node& stageRoot)
{
    if (!mGame) {
        return;
    }

    mGame->RemoveAllPlayer();

    YAML::Node playerNodes = stageRoot["players"];
    if (!playerNodes || !playerNodes.IsSequence() || playerNodes.size() == 0) {
        playerNodes = YAML::Node(YAML::NodeType::Sequence);
        YAML::Node defaultPlayer;
        defaultPlayer["currentPlanetNum"] = 0;
        playerNodes.push_back(defaultPlayer);
    }

    constexpr int requiredPlayerCount = 2;
    int loadedPlayerCount = 0;
    for (const YAML::Node& node : playerNodes) {
        if (loadedPlayerCount >= requiredPlayerCount) {
            break;
        }

        const int playerNum = loadedPlayerCount + 1;
        if (CreatePlayerFromStageNode(node, playerNum)) {
            loadedPlayerCount++;
        }
    }

    if (loadedPlayerCount >= requiredPlayerCount) {
        return;
    }

    const YAML::Node fallbackNode = playerNodes[0];
    while (loadedPlayerCount < requiredPlayerCount) {
        const int playerNum = loadedPlayerCount + 1;
        Player* duplicatedPlayer =
            CreatePlayerFromStageNode(fallbackNode, playerNum);
        if (!duplicatedPlayer) {
            break;
        }

        const std::vector<Player*>& players = mGame->GetPlayers();
        Player* firstPlayer = players.empty() ? nullptr : players[0];
        if (firstPlayer && duplicatedPlayer != firstPlayer) {
            glm::vec3 separationDirection = firstPlayer->GetLeftVec();
            const float separationLength = glm::length(separationDirection);
            if (separationLength > 0.000001f) {
                separationDirection /= separationLength;
                constexpr float duplicateSpawnSpacing = 1.0f;
                duplicatedPlayer->SetPos(
                    firstPlayer->GetPos() +
                    separationDirection * duplicateSpawnSpacing);
                duplicatedPlayer->Initialize();
                duplicatedPlayer->RefreshFallbackUpVec();
            }
        }

        loadedPlayerCount++;
    }
}

Player* StagePlayerLoader::CreatePlayerFromStageNode(
    const YAML::Node& node,
    int playerNum)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    const auto& planets = mGame->GetCurrentStage()->GetPlanets();
    const int currentPlanetNum =
        node["currentPlanetNum"].as<int>(0);
    if (currentPlanetNum < 0 ||
        currentPlanetNum >= static_cast<int>(planets.size())) {
        return nullptr;
    }

    Planet* currentPlanet = planets[currentPlanetNum];
    if (!currentPlanet) {
        return nullptr;
    }

    std::unique_ptr<Player> player = std::make_unique<Player>(mGame);
    player->SetPlayerNum(playerNum);
    player->SetCurrentPlanetNum(currentPlanetNum);
    player->SetCurrentPlanet(currentPlanet);

    mPlacementLoader.ApplyPlacementFromStageNode(
        player.get(),
        node,
        currentPlanet,
        playerNum - 1,
        0.0f);
    mPlacementLoader.ApplyRotationFromStageNode(player.get(), node);

    const glm::vec3 playerFacingDirection =
        -player->Actor::GetForwardVec();
    player->SetFacingForwardVec(playerFacingDirection);
    player->SetCameraForwardDirection(
        -playerFacingDirection,
        player->GetUpVec());

    player->ApplyConfig();
    if (node["modelPath"]) {
        player->SetModelPath(node["modelPath"].as<std::string>());
    }
    mPlacementLoader.ApplyScaleFromStageNode(player.get(), node);

    player->Initialize();
    player->SetBaseScale(player->GetScale());

    Player* playerPtr = player.get();
    mGame->GetMeshLoadSystem()->SetActorMesh(playerPtr);
    mGame->AddActor(std::move(player));
    mGame->AddPlayer(playerPtr);
    return playerPtr;
}

bool StagePlayerLoader::CreatePlayerFromCurrentStage(int playerNum)
{
    if (!mGame) {
        return false;
    }

    const YAML::Node root =
        YAML::LoadFile(mGame->GetCurrentStageYamlPath());
    if (!root["players"] || !root["players"].IsSequence()) {
        return false;
    }

    const int playerIndex = playerNum - 1;
    if (playerIndex < 0 ||
        playerIndex >= static_cast<int>(root["players"].size())) {
        return false;
    }

    CreatePlayerFromStageNode(root["players"][playerIndex], playerNum);
    return true;
}
