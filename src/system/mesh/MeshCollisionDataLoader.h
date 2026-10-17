#pragma once

#include <vector>

// モデルから衝突判定用の頂点・三角形インデックスを読み込む。GPUリソースは作成しない。
class MeshCollisionDataLoader {
public:
    // 出力をクリアしてXYZ順の頂点列と三角形インデックスを連結する。falseでも途中の出力が残る場合がある。
    bool LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                     std::vector<unsigned int>& outIndices) const;
};
