#include "system/mesh/AssimpMeshLoader.h"

#include "system/mesh/TextureLoader.h"

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <vector>

AssimpMeshLoader::AssimpMeshLoader(const TextureLoader* textureLoader)
    : mTextureLoader(textureLoader)
{
}

std::vector<LoadedMesh> AssimpMeshLoader::LoadMeshFromFile(const char* path) const
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals);

    std::vector<LoadedMesh> results;
    if (!scene) {
        return results;
    }

    for (unsigned int meshId = 0; meshId < scene->mNumMeshes; meshId++) {
        const aiMesh* mesh = scene->mMeshes[meshId];
        if (!mesh) {
            return results;
        }

        const bool hasNormals = (mesh->mNormals != nullptr);
        const bool hasUV = (mesh->mTextureCoords[0] != nullptr);

        std::vector<float> vertices;
        for (unsigned int vertexId = 0; vertexId < mesh->mNumVertices; vertexId++) {
            vertices.emplace_back(mesh->mVertices[vertexId].x);
            vertices.emplace_back(mesh->mVertices[vertexId].y);
            vertices.emplace_back(mesh->mVertices[vertexId].z);

            if (hasNormals) {
                vertices.emplace_back(mesh->mNormals[vertexId].x);
                vertices.emplace_back(mesh->mNormals[vertexId].y);
                vertices.emplace_back(mesh->mNormals[vertexId].z);
            }

            if (hasUV) {
                vertices.emplace_back(mesh->mTextureCoords[0][vertexId].x);
                vertices.emplace_back(mesh->mTextureCoords[0][vertexId].y);
            }
        }

        std::vector<unsigned int> indices;
        for (unsigned int faceId = 0; faceId < mesh->mNumFaces; faceId++) {
            const aiFace& face = mesh->mFaces[faceId];
            for (unsigned int indexId = 0; indexId < face.mNumIndices; indexId++) {
                indices.emplace_back(face.mIndices[indexId]);
            }
        }

        unsigned int VAO = 0;
        unsigned int VBO = 0;
        unsigned int EBO = 0;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        const int stride = 3 + (hasNormals ? 3 : 0) + (hasUV ? 2 : 0);
        int offset = 0;

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), reinterpret_cast<void*>(offset * sizeof(float)));
        glEnableVertexAttribArray(0);
        offset += 3;

        if (hasNormals) {
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), reinterpret_cast<void*>(offset * sizeof(float)));
            glEnableVertexAttribArray(1);
            offset += 3;
        }

        if (hasUV) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), reinterpret_cast<void*>(offset * sizeof(float)));
            glEnableVertexAttribArray(2);
        }

        glBindVertexArray(0);

        LoadedMesh result;
        result.VAO = VAO;
        result.indexCount = static_cast<unsigned int>(indices.size());

        if (scene->mMaterials && mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            aiColor3D diffuseColor;
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) == aiReturn_SUCCESS) {
                result.diffuseColor[0] = diffuseColor.r;
                result.diffuseColor[1] = diffuseColor.g;
                result.diffuseColor[2] = diffuseColor.b;
            }

            aiString texturePath;
            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) == aiReturn_SUCCESS && mTextureLoader) {
                std::string basePath(path);
                const size_t lastSlash = basePath.find_last_of("/\\");
                const std::string directory = (lastSlash != std::string::npos) ? basePath.substr(0, lastSlash + 1) : "";
                const std::string fullPath = directory + texturePath.C_Str();
                result.textureID = mTextureLoader->LoadTexture(fullPath.c_str());
            }
        }

        results.emplace_back(result);
    }

    return results;
}
