#pragma once

#include "system/mesh/AssimpMeshLoader.h"
#include "system/mesh/LoadedModel.h"
#include "system/mesh/MeshCollisionDataLoader.h"
#include "system/mesh/TextureLoader.h"

#include <string>
#include <unordered_map>
#include <vector>

class Actor;
class Game;

class MeshLoadSystem {
public:
    explicit MeshLoadSystem(Game* game);

    void Initialize();
    void SetActorMesh(Actor* actor);

    LoadedModel LoadModelFromFile(const char* path);

    bool LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                     std::vector<unsigned int>& outIndices);

    const LoadedModel* FindLoadedModel(const std::string& modelPath) const;

private:
    void CreateLoadedModels();
    void RegisterModel(const std::string& modelPath);

    LoadedModel* FindOrLoadModel(const std::string& modelPath);
    std::string ResolveModelFilePath(const std::string& modelPath) const;

private:
    Game* mGame;

    TextureLoader mTextureLoader;
    AssimpMeshLoader mAssimpMeshLoader;
    MeshCollisionDataLoader mCollisionDataLoader;

    std::unordered_map<std::string, LoadedModel> mLoadedModels;
};
