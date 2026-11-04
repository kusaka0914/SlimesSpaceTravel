#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <vector>

// 静的メッシュの半透明描画で使う、モデル空間の三角形中心と元の頂点インデックス。
struct MeshTriangle {
    glm::vec3 center{0.0f};
    std::array<unsigned int, 3> indices{};
};

// 描画用GPUハンドルとマテリアル値。キャッシュ内の資源はMeshLoadSystem::Shutdownで解放する。
// この構造体のコピー・破棄ではGPU資源を複製・解放しない。
struct LoadedMesh {
    unsigned int VAO = 0;
    unsigned int VBO = 0;
    unsigned int EBO = 0;

    unsigned int indexCount = 0;
    unsigned int textureID = 0;

    float diffuseColor[3] = {
        1.0f,
        1.0f,
        1.0f,
    };

    bool hasBoneInfluences = false;

    // スキニングしないメッシュだけに保持する。LoadedMeshのコピー間ではCPUデータも共有する。
    std::shared_ptr<const std::vector<MeshTriangle>> triangles;
};
