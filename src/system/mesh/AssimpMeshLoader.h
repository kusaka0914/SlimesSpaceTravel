#pragma once

#include "system/mesh/LoadedModel.h"

class TextureLoader;

class AssimpMeshLoader {
public:
    explicit AssimpMeshLoader(const TextureLoader* textureLoader);

    LoadedModel LoadModelFromFile(const char* path) const;

private:
    const TextureLoader* mTextureLoader;
};
