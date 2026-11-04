#include "gfx/render3d/TransparentMeshDrawOrder.h"

#include "system/mesh/LoadedMesh.h"

#include <algorithm>
#include <cmath>

TransparentMeshDrawOrder CreateTransparentMeshDrawOrder(std::span<const LoadedMesh> meshes, const glm::mat4& modelView)
{
    struct TriangleDraw {
        const LoadedMesh* mesh;
        const MeshTriangle* triangle;
        float viewDepth;
    };
    std::size_t triangleCount = 0;
    for (const LoadedMesh& mesh : meshes) {
        if (mesh.hasBoneInfluences || !mesh.triangles || mesh.triangles->size() * 3 != mesh.indexCount) {
            return {};
        }
        triangleCount += mesh.triangles->size();
    }

    std::vector<TriangleDraw> triangles;
    triangles.reserve(triangleCount);
    for (const LoadedMesh& mesh : meshes) {
        for (const MeshTriangle& triangle : *mesh.triangles) {
            const float depth = (modelView * glm::vec4(triangle.center, 1.0f)).z;
            if (!std::isfinite(depth)) {
                return {};
            }
            triangles.push_back({&mesh, &triangle, depth});
        }
    }
    // 視点空間では奥ほどZが小さい。同じ深度は元の順番を維持する。
    std::stable_sort(triangles.begin(), triangles.end(), [](const TriangleDraw& first, const TriangleDraw& second) {
        return first.viewDepth < second.viewDepth;
    });

    TransparentMeshDrawOrder order;
    order.indices.reserve(triangleCount * 3);
    for (const TriangleDraw& draw : triangles) {
        if (order.batches.empty() || order.batches.back().mesh != draw.mesh) {
            order.batches.push_back({draw.mesh, order.indices.size(), 0});
        }
        order.indices.insert(order.indices.end(), draw.triangle->indices.begin(), draw.triangle->indices.end());
        order.batches.back().indexCount += 3;
    }
    return order;
}
