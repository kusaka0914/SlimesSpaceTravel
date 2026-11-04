#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <span>
#include <vector>

struct LoadedMesh;

struct TransparentMeshBatch {
    const LoadedMesh* mesh = nullptr;
    std::size_t firstIndex = 0;
    std::size_t indexCount = 0;
};

// メッシュへの参照は描画中だけ借用する。indicesは各メッシュのVAO内の頂点番号。
struct TransparentMeshDrawOrder {
    std::vector<unsigned int> indices;
    std::vector<TransparentMeshBatch> batches;
};

// 静的モデルの全メッシュをまたいで三角形を奥から手前へ並べる。
// modelViewはモデル→視点空間の変換。スキニングや三角形情報不足では空を返し、呼出元の通常描画へ任せる。
TransparentMeshDrawOrder CreateTransparentMeshDrawOrder(std::span<const LoadedMesh> meshes, const glm::mat4& modelView);
