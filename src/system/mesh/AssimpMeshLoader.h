#pragma once

#include "system/mesh/LoadedMesh.h"

#include <vector>

class TextureLoader;

class AssimpMeshLoader {
public:
    explicit AssimpMeshLoader(const TextureLoader* textureLoader);

    std::vector<LoadedMesh> LoadMeshFromFile(const char* path) const;

private:
    const TextureLoader* mTextureLoader;
};
