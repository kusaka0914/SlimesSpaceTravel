#pragma once

#include "system/mesh/AssimpMeshLoader.h"
#include "system/mesh/LoadedModel.h"
#include "system/mesh/MeshCollisionDataLoader.h"
#include "system/mesh/TextureLoader.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Actor;
class Game;

struct CollisionMeshGeometry {
    std::vector<float> positions;
    std::vector<unsigned int> indices;
};

class MeshLoadSystem {
public:
    explicit MeshLoadSystem(Game* game);

    void Initialize();
    void SetActorMesh(Actor* actor);

    LoadedModel LoadModelFromFile(const char* path);

    const CollisionMeshGeometry* ResolveCollisionMeshGeometry(
        const std::string& modelFilePath);

    const LoadedModel* FindLoadedModel(const std::string& modelPath) const;
    const LoadedModel* ResolveLoadedModel(const std::string& modelPath);

private:
    void CreateLoadedModels();
    void RegisterModel(const std::string& modelPath);

    std::string ResolveModelFilePath(const std::string& modelPath) const;

private:
    Game* mGame;

    TextureLoader mTextureLoader;
    AssimpMeshLoader mAssimpMeshLoader;
    MeshCollisionDataLoader mCollisionDataLoader;

    std::unordered_map<std::string, LoadedModel> mLoadedModels;
    std::unordered_map<std::string, CollisionMeshGeometry> mCollisionMeshGeometryByPath;
    std::unordered_set<std::string> mFailedCollisionMeshPaths;
};
