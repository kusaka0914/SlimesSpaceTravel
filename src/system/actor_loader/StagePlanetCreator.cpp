#include "system/actor_loader/StagePlanetCreator.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "system/MeshLoadSystem.h"

#include <memory>
#include <utility>

StagePlanetCreator::StagePlanetCreator(Game* game)
    : mGame(game)
{
}

Planet* StagePlanetCreator::CreateFromStageNode(const YAML::Node& node)
{
    if (!mGame || !mGame->GetCurrentStage()) {
        return nullptr;
    }

    std::unique_ptr<Planet> planet = std::make_unique<Planet>(mGame);

    Stage* currentStage = mGame->GetCurrentStage();

    planet->SetCurrentStage(currentStage);
    planet->ApplyConfig(node);

    planet->Initialize();

    Planet* createdPlanet = planet.get();

    mGame->GetMeshLoadSystem()->SetActorMesh(createdPlanet);
    mGame->AddActor(std::move(planet));
    currentStage->AddPlanet(createdPlanet);

    return createdPlanet;
}
