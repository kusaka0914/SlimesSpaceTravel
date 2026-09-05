#include "system/mesh/AssimpMeshLoader.h"

#include "animation/SkeletalAnimationConstants.h"
#include "system/mesh/MeshVertex.h"
#include "system/mesh/TextureLoader.h"

#include <GL/glew.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr std::size_t InvalidBoneIndex = std::numeric_limits<std::size_t>::max();

glm::mat4 ConvertMatrix(const aiMatrix4x4& matrix)
{
    return glm::mat4(matrix.a1, matrix.b1, matrix.c1, matrix.d1, matrix.a2, matrix.b2, matrix.c2, matrix.d2,
                     matrix.a3, matrix.b3, matrix.c3, matrix.d3, matrix.a4, matrix.b4, matrix.c4, matrix.d4);
}

glm::vec3 ConvertVector(const aiVector3D& vector)
{
    return glm::vec3(vector.x, vector.y, vector.z);
}

glm::quat ConvertQuaternion(const aiQuaternion& quaternion)
{
    return glm::quat(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
}

SkeletonNode LoadSkeletonNode(const aiNode* sourceNode)
{
    SkeletonNode node;
    if (!sourceNode) {
        return node;
    }

    node.name = sourceNode->mName.C_Str();
    node.localBindTransform = ConvertMatrix(sourceNode->mTransformation);

    aiVector3D bindScale;
    aiQuaternion bindRotation;
    aiVector3D bindPosition;
    sourceNode->mTransformation.Decompose(bindScale, bindRotation, bindPosition);

    node.bindPosition = ConvertVector(bindPosition);
    node.bindRotation = ConvertQuaternion(bindRotation);
    node.bindScale = ConvertVector(bindScale);

    node.children.reserve(sourceNode->mNumChildren);
    for (unsigned int childIndex = 0; childIndex < sourceNode->mNumChildren; ++childIndex) {
        node.children.emplace_back(LoadSkeletonNode(sourceNode->mChildren[childIndex]));
    }

    return node;
}

AnimationClip LoadAnimationClip(const aiAnimation& sourceAnimation, unsigned int animationIndex)
{
    AnimationClip animationClip;
    animationClip.name = sourceAnimation.mName.length > 0
                             ? sourceAnimation.mName.C_Str()
                             : "Animation_" + std::to_string(animationIndex);
    animationClip.durationTicks = sourceAnimation.mDuration;
    animationClip.ticksPerSecond = sourceAnimation.mTicksPerSecond > 0.0 ? sourceAnimation.mTicksPerSecond : 25.0;

    for (unsigned int channelIndex = 0; channelIndex < sourceAnimation.mNumChannels; ++channelIndex) {
        const aiNodeAnim* sourceChannel = sourceAnimation.mChannels[channelIndex];
        if (!sourceChannel) {
            continue;
        }

        BoneAnimationChannel channel;
        channel.nodeName = sourceChannel->mNodeName.C_Str();

        channel.positionKeys.reserve(sourceChannel->mNumPositionKeys);
        for (unsigned int keyIndex = 0; keyIndex < sourceChannel->mNumPositionKeys; ++keyIndex) {
            const aiVectorKey& sourceKey = sourceChannel->mPositionKeys[keyIndex];
            channel.positionKeys.push_back({ConvertVector(sourceKey.mValue), sourceKey.mTime});
        }

        channel.rotationKeys.reserve(sourceChannel->mNumRotationKeys);
        for (unsigned int keyIndex = 0; keyIndex < sourceChannel->mNumRotationKeys; ++keyIndex) {
            const aiQuatKey& sourceKey = sourceChannel->mRotationKeys[keyIndex];
            channel.rotationKeys.push_back({ConvertQuaternion(sourceKey.mValue), sourceKey.mTime});
        }

        channel.scaleKeys.reserve(sourceChannel->mNumScalingKeys);
        for (unsigned int keyIndex = 0; keyIndex < sourceChannel->mNumScalingKeys; ++keyIndex) {
            const aiVectorKey& sourceKey = sourceChannel->mScalingKeys[keyIndex];
            channel.scaleKeys.push_back({ConvertVector(sourceKey.mValue), sourceKey.mTime});
        }

        animationClip.channelsByNodeName.insert_or_assign(channel.nodeName, std::move(channel));
    }

    return animationClip;
}

void LoadAnimationClips(const aiScene& scene, SkeletalAnimationData& animationData)
{
    animationData.clips.reserve(scene.mNumAnimations);
    for (unsigned int animationIndex = 0; animationIndex < scene.mNumAnimations; ++animationIndex) {
        const aiAnimation* sourceAnimation = scene.mAnimations[animationIndex];
        if (!sourceAnimation) {
            continue;
        }

        animationData.clips.emplace_back(LoadAnimationClip(*sourceAnimation, animationIndex));
    }
}

std::size_t FindOrAddBone(const aiBone& sourceBone, SkeletalAnimationData& animationData, bool& didExceedBoneLimit)
{
    const std::string boneName = sourceBone.mName.C_Str();
    const auto existingBoneIt = animationData.boneIndexByName.find(boneName);
    if (existingBoneIt != animationData.boneIndexByName.end()) {
        return existingBoneIt->second;
    }

    if (animationData.bones.size() >= SkeletalAnimationConstants::MaxShaderBoneCount) {
        didExceedBoneLimit = true;
        return InvalidBoneIndex;
    }

    const std::size_t boneIndex = animationData.bones.size();
    animationData.bones.push_back({boneName, ConvertMatrix(sourceBone.mOffsetMatrix)});
    animationData.boneIndexByName.emplace(boneName, boneIndex);
    return boneIndex;
}

void AddBoneInfluence(MeshVertex& vertex, int boneIndex, float boneWeight)
{
    if (boneWeight <= 0.0f) {
        return;
    }

    for (std::size_t influenceIndex = 0; influenceIndex < vertex.boneWeights.size(); ++influenceIndex) {
        if (vertex.boneWeights[influenceIndex] == 0.0f) {
            vertex.boneIndices[influenceIndex] = boneIndex;
            vertex.boneWeights[influenceIndex] = boneWeight;
            return;
        }
    }

    const auto smallestWeightIt = std::min_element(vertex.boneWeights.begin(), vertex.boneWeights.end());
    if (smallestWeightIt == vertex.boneWeights.end() || boneWeight <= *smallestWeightIt) {
        return;
    }

    const std::size_t influenceIndex = static_cast<std::size_t>(smallestWeightIt - vertex.boneWeights.begin());
    vertex.boneIndices[influenceIndex] = boneIndex;
    vertex.boneWeights[influenceIndex] = boneWeight;
}

void NormalizeBoneWeights(MeshVertex& vertex)
{
    const float totalWeight = std::accumulate(vertex.boneWeights.begin(), vertex.boneWeights.end(), 0.0f);
    if (totalWeight <= 0.0f) {
        return;
    }

    for (float& boneWeight : vertex.boneWeights) {
        boneWeight /= totalWeight;
    }
}

void LoadBoneInfluences(const aiMesh& sourceMesh, std::vector<MeshVertex>& vertices,
                        SkeletalAnimationData& animationData, bool& didExceedBoneLimit)
{
    for (unsigned int sourceBoneIndex = 0; sourceBoneIndex < sourceMesh.mNumBones; ++sourceBoneIndex) {
        const aiBone* sourceBone = sourceMesh.mBones[sourceBoneIndex];
        if (!sourceBone) {
            continue;
        }

        const std::size_t boneIndex = FindOrAddBone(*sourceBone, animationData, didExceedBoneLimit);
        if (boneIndex == InvalidBoneIndex) {
            continue;
        }

        for (unsigned int weightIndex = 0; weightIndex < sourceBone->mNumWeights; ++weightIndex) {
            const aiVertexWeight& sourceWeight = sourceBone->mWeights[weightIndex];
            if (sourceWeight.mVertexId >= vertices.size()) {
                continue;
            }

            AddBoneInfluence(vertices[sourceWeight.mVertexId], static_cast<int>(boneIndex), sourceWeight.mWeight);
        }
    }

    for (MeshVertex& vertex : vertices) {
        NormalizeBoneWeights(vertex);
    }
}

unsigned int LoadDiffuseTexture(const aiScene& scene, const aiMesh& sourceMesh, const char* modelPath,
                                const TextureLoader* textureLoader)
{
    if (!scene.mMaterials || sourceMesh.mMaterialIndex >= scene.mNumMaterials || !textureLoader) {
        return 0;
    }

    const aiMaterial* material = scene.mMaterials[sourceMesh.mMaterialIndex];
    if (!material) {
        return 0;
    }

    aiString texturePath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath) != aiReturn_SUCCESS) {
        return 0;
    }

    const std::string relativeTexturePath = texturePath.C_Str();
    if (relativeTexturePath.empty() || relativeTexturePath.front() == '*') {

        return 0;
    }

    const std::string basePath = modelPath ? modelPath : "";
    const std::size_t lastSlash = basePath.find_last_of("/\\");
    const std::string directory = lastSlash != std::string::npos ? basePath.substr(0, lastSlash + 1) : "";
    const std::string fullTexturePath = directory + relativeTexturePath;
    return textureLoader->LoadTexture(
        fullTexturePath.c_str(),
        TextureColorSpace::SRGB);
}

LoadedMesh CreateGpuMesh(const aiScene& scene, const aiMesh& sourceMesh, const char* modelPath,
                         const TextureLoader* textureLoader, SkeletalAnimationData& animationData,
                         bool& didExceedBoneLimit)
{
    std::vector<MeshVertex> vertices(sourceMesh.mNumVertices);

    const bool hasNormals = sourceMesh.HasNormals();
    const bool hasTextureCoordinates = sourceMesh.HasTextureCoords(0);

    for (unsigned int vertexIndex = 0; vertexIndex < sourceMesh.mNumVertices; ++vertexIndex) {
        MeshVertex& vertex = vertices[vertexIndex];
        vertex.position = {sourceMesh.mVertices[vertexIndex].x, sourceMesh.mVertices[vertexIndex].y,
                           sourceMesh.mVertices[vertexIndex].z};

        if (hasNormals) {
            vertex.normal = {sourceMesh.mNormals[vertexIndex].x, sourceMesh.mNormals[vertexIndex].y,
                             sourceMesh.mNormals[vertexIndex].z};
        }

        if (hasTextureCoordinates) {
            vertex.textureCoordinate = {sourceMesh.mTextureCoords[0][vertexIndex].x,
                                        sourceMesh.mTextureCoords[0][vertexIndex].y};
        }
    }

    LoadBoneInfluences(sourceMesh, vertices, animationData, didExceedBoneLimit);

    std::vector<unsigned int> indices;
    indices.reserve(sourceMesh.mNumFaces * 3);
    for (unsigned int faceIndex = 0; faceIndex < sourceMesh.mNumFaces; ++faceIndex) {
        const aiFace& face = sourceMesh.mFaces[faceIndex];
        for (unsigned int indexIndex = 0; indexIndex < face.mNumIndices; ++indexIndex) {
            indices.emplace_back(face.mIndices[indexIndex]);
        }
    }

    LoadedMesh loadedMesh;
    glGenVertexArrays(1, &loadedMesh.VAO);
    glGenBuffers(1, &loadedMesh.VBO);
    glGenBuffers(1, &loadedMesh.EBO);

    glBindVertexArray(loadedMesh.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, loadedMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(MeshVertex)), vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, loadedMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, normal)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, textureCoordinate)));
    glEnableVertexAttribArray(2);

    glVertexAttribIPointer(3, 4, GL_INT, sizeof(MeshVertex),
                           reinterpret_cast<void*>(offsetof(MeshVertex, boneIndices)));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(MeshVertex),
                          reinterpret_cast<void*>(offsetof(MeshVertex, boneWeights)));
    glEnableVertexAttribArray(4);

    glBindVertexArray(0);

    loadedMesh.indexCount = static_cast<unsigned int>(indices.size());
    loadedMesh.textureID = LoadDiffuseTexture(scene, sourceMesh, modelPath, textureLoader);
    loadedMesh.hasBoneInfluences = std::any_of(vertices.begin(), vertices.end(), [](const MeshVertex& vertex) {
        return std::any_of(vertex.boneWeights.begin(), vertex.boneWeights.end(),
                           [](float boneWeight) { return boneWeight > 0.0f; });
    });

    if (scene.mMaterials && sourceMesh.mMaterialIndex < scene.mNumMaterials) {
        const aiMaterial* material = scene.mMaterials[sourceMesh.mMaterialIndex];
        aiColor4D baseColor;


        if (material &&
            material->Get(AI_MATKEY_BASE_COLOR, baseColor) ==
                aiReturn_SUCCESS) {
            loadedMesh.diffuseColor[0] = baseColor.r;
            loadedMesh.diffuseColor[1] = baseColor.g;
            loadedMesh.diffuseColor[2] = baseColor.b;
        } else {
            aiColor3D diffuseColor;
            if (material &&
                material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor) ==
                    aiReturn_SUCCESS) {
                loadedMesh.diffuseColor[0] = diffuseColor.r;
                loadedMesh.diffuseColor[1] = diffuseColor.g;
                loadedMesh.diffuseColor[2] = diffuseColor.b;
            }
        }
    }

    return loadedMesh;
}
}

AssimpMeshLoader::AssimpMeshLoader(const TextureLoader* textureLoader)
    : mTextureLoader(textureLoader)
{
}

LoadedModel AssimpMeshLoader::LoadModelFromFile(const char* path) const
{
    LoadedModel loadedModel;
    if (!path) {
        return loadedModel;
    }

    Assimp::Importer importer;
    constexpr unsigned int importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_LimitBoneWeights;
    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene || !scene->mRootNode) {
        std::cerr << "Failed to load model '" << path << "': " << importer.GetErrorString() << '\n';
        return loadedModel;
    }

    loadedModel.skeletalAnimation.rootNode = LoadSkeletonNode(scene->mRootNode);
    loadedModel.skeletalAnimation.inverseRootTransform = glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));

    bool didExceedBoneLimit = false;
    loadedModel.meshes.reserve(scene->mNumMeshes);
    for (unsigned int meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
        const aiMesh* sourceMesh = scene->mMeshes[meshIndex];
        if (!sourceMesh) {
            continue;
        }

        for (unsigned int vertexIndex = 0;
             vertexIndex < sourceMesh->mNumVertices;
             ++vertexIndex) {
            const aiVector3D& sourcePosition =
                sourceMesh->mVertices[vertexIndex];
            const glm::vec3 position(
                sourcePosition.x,
                sourcePosition.y,
                sourcePosition.z);
            if (!loadedModel.hasBounds) {
                loadedModel.boundsMinimum = position;
                loadedModel.boundsMaximum = position;
                loadedModel.hasBounds = true;
                continue;
            }

            loadedModel.boundsMinimum = glm::min(
                loadedModel.boundsMinimum,
                position);
            loadedModel.boundsMaximum = glm::max(
                loadedModel.boundsMaximum,
                position);
        }

        loadedModel.meshes.emplace_back(CreateGpuMesh(*scene, *sourceMesh, path, mTextureLoader,
                                                      loadedModel.skeletalAnimation, didExceedBoneLimit));
    }

    LoadAnimationClips(*scene, loadedModel.skeletalAnimation);

    if (didExceedBoneLimit) {
        std::cerr << "Model '" << path << "' uses more than " << SkeletalAnimationConstants::MaxShaderBoneCount
                  << " skinning bones. Influences above the shader limit were ignored.\n";
    }

    return loadedModel;
}
