#pragma once

#include <vector>

class MeshCollisionDataLoader {
public:
    bool LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                     std::vector<unsigned int>& outIndices) const;
};
