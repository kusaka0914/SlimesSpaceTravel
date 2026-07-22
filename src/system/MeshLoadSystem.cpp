#include "system/MeshLoadSystem.h"

#include "actor/Actor.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

MeshLoadSystem::MeshLoadSystem(Game* game)
    : mGame(game),
      mAssimpMeshLoader(&mTextureLoader)
{
    Initialize();
}

void MeshLoadSystem::Initialize()
{
    CreateLoadedModels();
}

void MeshLoadSystem::CreateLoadedModels()
{
    RegisterModel("player.obj");
    RegisterModel("enemy.obj");
    RegisterModel("key.obj");
    RegisterModel("star.obj");
    RegisterModel("spaceSlime.obj");
    RegisterModel("selectField.obj");
    RegisterModel("planet.obj");
    RegisterModel("planet_2.obj");
    RegisterModel("planet_3.obj");
    RegisterModel("planet7.obj");
    RegisterModel("planet9.obj");
    RegisterModel("house.obj");
    RegisterModel("badMotherSlime.obj");
    RegisterModel("boat.obj");
    RegisterModel("rocketParts1.obj");
    RegisterModel("rocketParts2.obj");
    RegisterModel("rocketParts3.obj");
    RegisterModel("rocketParts4.obj");
    RegisterModel("rocketParts5.obj");
    RegisterModel("crystals.obj");
    RegisterModel("doctorSlime.obj");
    RegisterModel("motherSlime.obj");
    RegisterModel("platform.obj");
    RegisterModel("curvePlatform.obj");
    RegisterModel("rocket.fbx");
    RegisterModel("skyBox.obj");
}

void MeshLoadSystem::RegisterModel(const std::string& modelPath)
{
    if (modelPath.empty() || mLoadedModels.contains(modelPath)) {
        return;
    }

    const std::string resolvedPath = ResolveModelFilePath(modelPath);
    mLoadedModels.emplace(modelPath, LoadModelFromFile(resolvedPath.c_str()));
}

void MeshLoadSystem::SetActorMesh(Actor* actor)
{
    if (!actor) {
        return;
    }

    LoadedModel* loadedModel = FindOrLoadModel(actor->GetModelPath());
    actor->SetLoadedModel(loadedModel);
}

LoadedModel MeshLoadSystem::LoadModelFromFile(const char* path)
{
    return mAssimpMeshLoader.LoadModelFromFile(path);
}

bool MeshLoadSystem::LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                                 std::vector<unsigned int>& outIndices)
{
    return mCollisionDataLoader.LoadMeshPositionsAndIndices(path, outPositions, outIndices);
}

const LoadedModel* MeshLoadSystem::FindLoadedModel(const std::string& modelPath) const
{
    const auto loadedModelIt = mLoadedModels.find(modelPath);
    return loadedModelIt != mLoadedModels.end() ? &loadedModelIt->second : nullptr;
}

LoadedModel* MeshLoadSystem::FindOrLoadModel(const std::string& modelPath)
{
    if (modelPath.empty()) {
        return nullptr;
    }

    const auto loadedModelIt = mLoadedModels.find(modelPath);
    if (loadedModelIt != mLoadedModels.end()) {
        return &loadedModelIt->second;
    }

    const std::string resolvedPath = ResolveModelFilePath(modelPath);
    auto [insertedModelIt, wasInserted] =
        mLoadedModels.emplace(modelPath, LoadModelFromFile(resolvedPath.c_str()));
    (void)wasInserted;
    return &insertedModelIt->second;
}

std::string MeshLoadSystem::ResolveModelFilePath(const std::string& modelPath) const
{
    const std::filesystem::path requestedPath(modelPath);
    if (requestedPath.is_absolute()) {
        return requestedPath.lexically_normal().string();
    }

    return (std::filesystem::path("../assets/models") / requestedPath).lexically_normal().string();
}
