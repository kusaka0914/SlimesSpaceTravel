#include "MeshLoadSystem.h"

#include "actor/Actor.h"

#include <string>
#include <vector>

MeshLoadSystem::MeshLoadSystem(Game* game)
    : mGame(game),
      mAssimpMeshLoader(&mTextureLoader)
{
    Initialize();
}

void MeshLoadSystem::Initialize()
{
    CreateLoadedMeshes();
}

void MeshLoadSystem::CreateLoadedMeshes()
{
    RegisterMesh("player", "../assets/models/player.obj");
    RegisterMesh("enemy", "../assets/models/enemy.obj");
    RegisterMesh("key", "../assets/models/key.obj");
    RegisterMesh("star", "../assets/models/star.obj");
    RegisterMesh("spaceSlime", "../assets/models/spaceSlime.obj");
    RegisterMesh("selectField", "../assets/models/selectField.obj");
    RegisterMesh("planet", "../assets/models/planet.obj");
    RegisterMesh("planet_2", "../assets/models/planet_2.obj");
    RegisterMesh("planet_3", "../assets/models/planet_3.obj");
    RegisterMesh("planet7", "../assets/models/planet7.obj");
    RegisterMesh("planet9", "../assets/models/planet9.obj");
    RegisterMesh("house", "../assets/models/house.obj");
    RegisterMesh("badMotherSlime", "../assets/models/badMotherSlime.obj");
    RegisterMesh("boat", "../assets/models/boat.obj");
    RegisterMesh("rocketParts1", "../assets/models/rocketParts1.obj");
    RegisterMesh("rocketParts2", "../assets/models/rocketParts2.obj");
    RegisterMesh("rocketParts3", "../assets/models/rocketParts3.obj");
    RegisterMesh("rocketParts4", "../assets/models/rocketParts4.obj");
    RegisterMesh("rocketParts5", "../assets/models/rocketParts5.obj");
    RegisterMesh("crystals", "../assets/models/crystals.obj");
    RegisterMesh("doctorSlime", "../assets/models/doctorSlime.obj");
    RegisterMesh("motherSlime", "../assets/models/motherSlime.obj");
    RegisterMesh("platform", "../assets/models/platform.obj");
    RegisterMesh("curvePlatform", "../assets/models/curvePlatform.obj");
    RegisterMesh("rocket", "../assets/models/rocket.fbx");
    RegisterMesh("skyBox", "../assets/models/skyBox.obj");
}

void MeshLoadSystem::RegisterMesh(const std::string& meshName, const char* path)
{
    mLoadedMeshes[meshName] = LoadMeshFromFile(path);
}

void MeshLoadSystem::SetActorMesh(Actor* actor)
{
    if (!actor) {
        return;
    }

    const auto dotPos = actor->GetModelPath().find('.');
    const std::string meshName = actor->GetModelPath().substr(0, dotPos);
    actor->SetMeshes(GetLoadedMeshes(meshName));
}

std::vector<LoadedMesh> MeshLoadSystem::LoadMeshFromFile(const char* path)
{
    return mAssimpMeshLoader.LoadMeshFromFile(path);
}

bool MeshLoadSystem::LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                                 std::vector<unsigned int>& outIndices)
{
    return mCollisionDataLoader.LoadMeshPositionsAndIndices(path, outPositions, outIndices);
}
