#pragma once

#include "animation/SkeletalAnimationConstants.h"

#include <array>

struct MeshVertex {
    std::array<float, 3> position{0.0f, 0.0f, 0.0f};
    std::array<float, 3> normal{0.0f, 0.0f, 0.0f};
    std::array<float, 2> textureCoordinate{0.0f, 0.0f};
    std::array<int, SkeletalAnimationConstants::MaxBoneInfluencesPerVertex> boneIndices{-1, -1, -1, -1};
    std::array<float, SkeletalAnimationConstants::MaxBoneInfluencesPerVertex> boneWeights{0.0f, 0.0f, 0.0f, 0.0f};
};
