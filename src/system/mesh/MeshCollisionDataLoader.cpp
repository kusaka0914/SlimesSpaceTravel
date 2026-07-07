#include "system/mesh/MeshCollisionDataLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

bool MeshCollisionDataLoader::LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                                          std::vector<unsigned int>& outIndices) const
{
    outPositions.clear();
    outIndices.clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene || !scene->mMeshes || scene->mNumMeshes == 0) {
        return false;
    }

    unsigned int vertexOffset = 0;
    for (unsigned int meshId = 0; meshId < scene->mNumMeshes; meshId++) {
        const aiMesh* mesh = scene->mMeshes[meshId];
        if (!mesh) {
            return false;
        }

        for (unsigned int vertexId = 0; vertexId < mesh->mNumVertices; vertexId++) {
            outPositions.emplace_back(mesh->mVertices[vertexId].x);
            outPositions.emplace_back(mesh->mVertices[vertexId].y);
            outPositions.emplace_back(mesh->mVertices[vertexId].z);
        }

        for (unsigned int faceId = 0; faceId < mesh->mNumFaces; faceId++) {
            const aiFace& face = mesh->mFaces[faceId];
            for (unsigned int indexId = 0; indexId < face.mNumIndices; indexId++) {
                outIndices.emplace_back(vertexOffset + face.mIndices[indexId]);
            }
        }

        vertexOffset += mesh->mNumVertices;
    }

    return true;
}
