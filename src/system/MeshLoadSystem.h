#pragma once

#include <iostream>
#include <unordered_map>
#include <vector>

struct LoadedMesh {
    unsigned int VAO = 0;
    unsigned int indexCount = 0;
    unsigned int textureID = 0;
    float diffuseColor[3] = {1.0f, 1.0f, 1.0f};
};

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

    std::vector<LoadedMesh>* GetLoadedMeshes(const std::string& MeshName)
    {
        auto it = mLoadedMeshes.find(MeshName);
        return (it != mLoadedMeshes.end()) ? &it->second : nullptr;
    }

private:
    void CreateLoadedMeshes();
    unsigned int loadTexture(const char* path);

private:
    Game* mGame;
    std::unordered_map<std::string, std::vector<LoadedMesh>> mLoadedMeshes;
};