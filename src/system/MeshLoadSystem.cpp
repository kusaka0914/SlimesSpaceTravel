#include <GL/glew.h>

#include "Game.h"
#include "MeshLoadSystem.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "thirdParty/stb_image.h"

MeshLoadSystem::MeshLoadSystem(Game* game)
    : mGame(game)
{
    Initialize();
}

void MeshLoadSystem::Initialize()
{
    CreateLoadedMeshes();
}

void MeshLoadSystem::CreateLoadedMeshes()
{
    mLoadedMeshes["player"] = LoadMeshFromFile("../assets/models/player.obj");
    mLoadedMeshes["enemy"] = LoadMeshFromFile("../assets/models/enemy.obj");
    mLoadedMeshes["key"] = LoadMeshFromFile("../assets/models/key.obj");
    mLoadedMeshes["star"] = LoadMeshFromFile("../assets/models/star.obj");
    mLoadedMeshes["spaceSlime"] = LoadMeshFromFile("../assets/models/spaceSlime.obj");
    mLoadedMeshes["selectField"] = LoadMeshFromFile("../assets/models/selectField.obj");
    mLoadedMeshes["planet"] = LoadMeshFromFile("../assets/models/planet.obj");
    mLoadedMeshes["planet_2"] = LoadMeshFromFile("../assets/models/planet_2.obj");
    mLoadedMeshes["planet_3"] = LoadMeshFromFile("../assets/models/planet_3.obj");
    mLoadedMeshes["planet7"] = LoadMeshFromFile("../assets/models/planet7.obj");
    mLoadedMeshes["planet9"] = LoadMeshFromFile("../assets/models/planet9.obj");
    mLoadedMeshes["house"] = LoadMeshFromFile("../assets/models/house.obj");
    mLoadedMeshes["badMotherSlime"] = LoadMeshFromFile("../assets/models/badMotherSlime.obj");
    mLoadedMeshes["boat"] = LoadMeshFromFile("../assets/models/boat.obj");
    mLoadedMeshes["rocketParts1"] = LoadMeshFromFile("../assets/models/rocketParts1.obj");
    mLoadedMeshes["rocketParts2"] = LoadMeshFromFile("../assets/models/rocketParts2.obj");
    mLoadedMeshes["rocketParts3"] = LoadMeshFromFile("../assets/models/rocketParts3.obj");
    mLoadedMeshes["rocketParts4"] = LoadMeshFromFile("../assets/models/rocketParts4.obj");
    mLoadedMeshes["rocketParts5"] = LoadMeshFromFile("../assets/models/rocketParts5.obj");
    mLoadedMeshes["crystals"] = LoadMeshFromFile("../assets/models/crystals.obj");
    mLoadedMeshes["doctorSlime"] = LoadMeshFromFile("../assets/models/doctorSlime.obj");
    mLoadedMeshes["motherSlime"] = LoadMeshFromFile("../assets/models/motherSlime.obj");
    mLoadedMeshes["enemy"] = LoadMeshFromFile("../assets/models/enemy.obj");
    mLoadedMeshes["platform"] = LoadMeshFromFile("../assets/models/platform.obj");
    mLoadedMeshes["curvePlatform"] = LoadMeshFromFile("../assets/models/curvePlatform.obj");
    mLoadedMeshes["rocket"] = LoadMeshFromFile("../assets/models/rocket.fbx");
    mLoadedMeshes["skyBox"] = LoadMeshFromFile("../assets/models/skyBox.obj");
}

void MeshLoadSystem::SetActorMesh(Actor* actor)
{
    auto it = actor->GetModelPath().find(".");
    const std::string& meshName = actor->GetModelPath().substr(0, it);
    actor->SetMeshes(GetLoadedMeshes(meshName));
}

std::vector<LoadedMesh> MeshLoadSystem::LoadMeshFromFile(const char* path)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals);

    std::vector<LoadedMesh> results;
    if (!scene) {
        // std::cerr << "Assimp error: " << importer.GetErrorString() << std::endl;
        return results;
    }

    for (unsigned int meshId = 0; meshId < scene->mNumMeshes; meshId++) {
        const aiMesh* mesh = scene->mMeshes[meshId];
        if (!mesh)
            return results;

        bool hasNormals = (mesh->mNormals != nullptr);
        bool hasUV = (mesh->mTextureCoords[0] != nullptr);

        std::vector<float> vertices;
        for (unsigned int verticeId = 0; verticeId < mesh->mNumVertices; verticeId++) {
            vertices.emplace_back(mesh->mVertices[verticeId].x);
            vertices.emplace_back(mesh->mVertices[verticeId].y);
            vertices.emplace_back(mesh->mVertices[verticeId].z);

            bool hasNormals = (mesh->mNormals != nullptr);
            if (hasNormals) {
                vertices.emplace_back(mesh->mNormals[verticeId].x);
                vertices.emplace_back(mesh->mNormals[verticeId].y);
                vertices.emplace_back(mesh->mNormals[verticeId].z);
            }

            bool hasUV = (mesh->mTextureCoords[0] != nullptr);
            if (hasUV) {
                vertices.emplace_back(mesh->mTextureCoords[0][verticeId].x);
                vertices.emplace_back(mesh->mTextureCoords[0][verticeId].y);
            }
        }

        std::vector<unsigned int> indices;
        for (unsigned int faceId = 0; faceId < mesh->mNumFaces; faceId++) {
            const aiFace& face = mesh->mFaces[faceId];
            for (unsigned int indiceId = 0; indiceId < face.mNumIndices; indiceId++)
                indices.emplace_back(face.mIndices[indiceId]);
        }

        unsigned int VAO, VBO, EBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        int stride = 3 + (hasNormals ? 3 : 0) + (hasUV ? 2 : 0);
        int offset = 0;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
        glEnableVertexAttribArray(0);
        offset += 3;
        if (hasNormals) {
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
            glEnableVertexAttribArray(1);
            offset += 3;
        }
        if (hasUV) {
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride * sizeof(float), (void*)(offset * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        glBindVertexArray(0);

        LoadedMesh result;
        result.VAO = VAO;
        result.indexCount = static_cast<unsigned int>(indices.size());

        if (scene->mMaterials && mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
            aiColor3D kd;
            if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, kd) == aiReturn_SUCCESS) {
                result.diffuseColor[0] = kd.r;
                result.diffuseColor[1] = kd.g;
                result.diffuseColor[2] = kd.b;
            }
            aiString texPath;
            if (mat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS) {
                std::string base(path);
                size_t lastSlash = base.find_last_of("/\\");
                std::string dir = (lastSlash != std::string::npos) ? base.substr(0, lastSlash + 1) : "";
                std::string fullPath = dir + texPath.C_Str();
                result.textureID = loadTexture(fullPath.c_str());
            }
        }
        results.emplace_back(result);
    }
    return results;
}

bool MeshLoadSystem::LoadMeshPositionsAndIndices(const char* path, std::vector<float>& outPositions,
                                                 std::vector<unsigned int>& outIndices)
{
    outPositions.clear();
    outIndices.clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate);

    if (!scene || !scene->mMeshes || scene->mNumMeshes == 0) {
        // std::cerr << "Assimp error (mesh positions): " << importer.GetErrorString() << std::endl;
        return false;
    }

    unsigned int vertexOffset = 0;
    for (unsigned int meshId = 0; meshId < scene->mNumMeshes; meshId++) {
        const aiMesh* mesh = scene->mMeshes[meshId];
        for (unsigned int verticeId = 0; verticeId < mesh->mNumVertices; verticeId++) {
            outPositions.emplace_back(mesh->mVertices[verticeId].x);
            outPositions.emplace_back(mesh->mVertices[verticeId].y);
            outPositions.emplace_back(mesh->mVertices[verticeId].z);
        }

        for (unsigned int faceId = 0; faceId < mesh->mNumFaces; faceId++) {
            const aiFace& face = mesh->mFaces[faceId];
            for (unsigned int indiceId = 0; indiceId < face.mNumIndices; indiceId++)
                outIndices.emplace_back(vertexOffset + face.mIndices[indiceId]);
        }
        vertexOffset += mesh->mNumVertices;
    }
    return true;
}

unsigned int MeshLoadSystem::loadTexture(const char* path)
{
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int w, h, nrChannels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path, &w, &h, &nrChannels, 0);
    if (!data) {
        // std::cerr << "Failed to load texture: " << path << std::endl;
        glDeleteTextures(1, &texID);
        return 0;
    }
    GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texID;
}