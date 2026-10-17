#pragma once

#include "animation/SkeletalAnimationData.h"
#include "system/mesh/LoadedMesh.h"

#include <glm/glm.hpp>
#include <vector>

// 描画メッシュ・骨格アニメーションと、モデル空間の頂点境界を保持する。
struct LoadedModel {
    std::vector<LoadedMesh> meshes;
    SkeletalAnimationData skeletalAnimation;
    glm::vec3 boundsMinimum = glm::vec3(0.0f);
    glm::vec3 boundsMaximum = glm::vec3(0.0f);
    bool hasBounds = false;

    bool IsLoaded() const { return !meshes.empty(); }
    bool HasSkinningData() const { return skeletalAnimation.HasSkinningData(); }
    bool HasAnimationClips() const { return skeletalAnimation.HasAnimationClips(); }
};
