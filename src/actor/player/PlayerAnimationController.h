#pragma once

#include "animation/AnimationPlayer.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

struct AnimationClip;
struct LoadedModel;

class PlayerAnimationController {
public:
    void Configure(std::string idleAnimationName, std::string attackAnimationName);
    void SetLoadedModel(const LoadedModel* loadedModel);

    void Update(bool didAttackStart, float deltaTimeSeconds);
    void ResetToIdle();

    const std::vector<glm::mat4>* GetSkinningMatrices() const;

private:
    void ResolveAnimationClips();
    const AnimationClip* FindAnimationClip(const std::string& requestedName, bool canUseSingleClipFallback) const;

private:
    const LoadedModel* mLoadedModel = nullptr;
    const AnimationClip* mIdleAnimationClip = nullptr;
    const AnimationClip* mAttackAnimationClip = nullptr;

    std::string mIdleAnimationName = "Idle";
    std::string mAttackAnimationName = "Attack";

    AnimationPlayer mAnimationPlayer;
};
