#pragma once

#include "system/mesh/AssimpMeshLoader.h"
#include "system/mesh/LoadedMesh.h"
#include "system/mesh/MeshCollisionDataLoader.h"
#include "system/mesh/TextureLoader.h"

#include <string>
#include <unordered_map>
#include <vector>

class Actor;
class Game;

class MeshLoadSystem {
public:
    MeshLoadSystem(Game* game);

    void Initialize();
    void SetActorMesh(Actor* actor);

    std::vector<LoadedMesh> LoadMeshFromFile(const char* path);

    bool LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                     std::vector<unsigned int>& outIndices);

    std::vector<LoadedMesh>* GetLoadedMeshes(const std::string& meshName)
    {
        auto it = mLoadedMeshes.find(meshName);
        return (it != mLoadedMeshes.end()) ? &it->second : nullptr;
    }

private:
    void CreateLoadedMeshes();
    void RegisterMesh(const std::string& meshName, const char* path);

private:
    Game* mGame;

    TextureLoader mTextureLoader;
    AssimpMeshLoader mAssimpMeshLoader;
    MeshCollisionDataLoader mCollisionDataLoader;

    std::unordered_map<std::string, std::vector<LoadedMesh>> mLoadedMeshes;
};
