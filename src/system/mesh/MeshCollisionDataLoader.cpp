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
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        if (!mesh) {
            return false;
        }

        for (unsigned int vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
            outPositions.emplace_back(mesh->mVertices[vertexIndex].x);
            outPositions.emplace_back(mesh->mVertices[vertexIndex].y);
            outPositions.emplace_back(mesh->mVertices[vertexIndex].z);
        }

        for (unsigned int faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
            const aiFace& face = mesh->mFaces[faceIndex];
            for (unsigned int indexIndex = 0; indexIndex < face.mNumIndices; indexIndex++) {
                outIndices.emplace_back(vertexOffset + face.mIndices[indexIndex]);
            }
        }

        vertexOffset += mesh->mNumVertices;
    }

    return true;
}
