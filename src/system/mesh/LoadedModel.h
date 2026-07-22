#pragma once

#include "animation/SkeletalAnimationData.h"
#include "system/mesh/LoadedMesh.h"

#include <vector>

struct LoadedModel {
    std::vector<LoadedMesh> meshes;
    SkeletalAnimationData skeletalAnimation;

    bool IsLoaded() const { return !meshes.empty(); }
    bool HasSkinningData() const { return skeletalAnimation.HasSkinningData(); }
    bool HasAnimationClips() const { return skeletalAnimation.HasAnimationClips(); }
};
