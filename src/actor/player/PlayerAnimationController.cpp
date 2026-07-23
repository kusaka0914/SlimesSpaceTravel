#include "actor/player/PlayerAnimationController.h"

#include "system/mesh/LoadedModel.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <utility>

namespace {
std::string NormalizeAnimationName(const std::string& animationName)
{
    std::string normalizedName;
    normalizedName.reserve(animationName.size());

    for (const unsigned char character : animationName) {
        if (!std::isspace(character)) {
            normalizedName.push_back(static_cast<char>(std::tolower(character)));
        }
    }

    return normalizedName;
}

std::string GetAnimationNameSuffix(const std::string& normalizedName)
{
    const std::size_t separatorPosition = normalizedName.find_last_of("|:/\\");
    return separatorPosition == std::string::npos ? normalizedName : normalizedName.substr(separatorPosition + 1);
}
} // namespace

void PlayerAnimationController::Configure(std::string idleAnimationName, std::string walkAnimationName,
                                          std::string attackAnimationName)
{
    mIdleAnimationName = std::move(idleAnimationName);
    mWalkAnimationName = std::move(walkAnimationName);
    mAttackAnimationName = std::move(attackAnimationName);

    if (mLoadedModel) {
        ResolveAnimationClips();
        ResetToIdle();
    }
}

void PlayerAnimationController::SetLoadedModel(const LoadedModel* loadedModel)
{
    mLoadedModel = loadedModel;

    const SkeletalAnimationData* animationData =
        mLoadedModel && mLoadedModel->HasSkinningData() ? &mLoadedModel->skeletalAnimation : nullptr;
    mAnimationPlayer.SetAnimationData(animationData);

    ResolveAnimationClips();
    ResetToIdle();
}

void PlayerAnimationController::Update(bool didAttackStart, bool shouldWalk, float deltaTimeSeconds)
{
    if (!mAnimationPlayer.HasSkinningData()) {
        return;
    }

    if (didAttackStart && mAttackAnimationClip) {
        mAnimationPlayer.Play(mAttackAnimationClip, false, true);
    } else {
        const bool isAttackPlaying =
            mAttackAnimationClip && mAnimationPlayer.IsPlaying(mAttackAnimationClip) && !mAnimationPlayer.IsFinished();

        if (!isAttackPlaying) {
            PlayLocomotionAnimation(shouldWalk);
        }
    }

    mAnimationPlayer.Update(deltaTimeSeconds);
}

void PlayerAnimationController::ResetToIdle()
{
    if (!mAnimationPlayer.HasSkinningData()) {
        return;
    }

    if (mIdleAnimationClip) {
        mAnimationPlayer.Play(mIdleAnimationClip, true, true);
        return;
    }

    mAnimationPlayer.ResetToBindPose();
}

const std::vector<glm::mat4>* PlayerAnimationController::GetSkinningMatrices() const
{
    return mAnimationPlayer.HasSkinningData() ? &mAnimationPlayer.GetFinalBoneTransforms() : nullptr;
}

void PlayerAnimationController::ResolveAnimationClips()
{
    mIdleAnimationClip = FindAnimationClip(mIdleAnimationName, false);
    mWalkAnimationClip = FindAnimationClip(mWalkAnimationName, false);
    mAttackAnimationClip = FindAnimationClip(mAttackAnimationName, true);

    if (!mLoadedModel || !mLoadedModel->HasAnimationClips()) {
        return;
    }

    if (!mIdleAnimationClip) {
        std::cerr << "Idle animation '" << mIdleAnimationName << "' was not found.\n";
    }

    if (!mWalkAnimationClip) {
        std::cerr << "Walk animation '" << mWalkAnimationName << "' was not found.\n";
    }

    if (!mAttackAnimationClip) {
        std::cerr << "Attack animation '" << mAttackAnimationName << "' was not found. Available clips:";
        for (const AnimationClip& animationClip : mLoadedModel->skeletalAnimation.clips) {
            std::cerr << " '" << animationClip.name << "'";
        }
        std::cerr << '\n';
    }
}

void PlayerAnimationController::PlayLocomotionAnimation(bool shouldWalk)
{
    const AnimationClip* locomotionAnimationClip =
        shouldWalk && mWalkAnimationClip ? mWalkAnimationClip : mIdleAnimationClip;

    if (!locomotionAnimationClip) {
        if (mAnimationPlayer.GetCurrentClip()) {
            mAnimationPlayer.ResetToBindPose();
        }
        return;
    }

    if (!mAnimationPlayer.IsPlaying(locomotionAnimationClip)) {
        mAnimationPlayer.Play(locomotionAnimationClip, true, true);
    }
}

const AnimationClip* PlayerAnimationController::FindAnimationClip(const std::string& requestedName,
                                                                   bool canUseSingleClipFallback) const
{
    if (!mLoadedModel || !mLoadedModel->HasAnimationClips()) {
        return nullptr;
    }

    const std::vector<AnimationClip>& animationClips = mLoadedModel->skeletalAnimation.clips;
    if (canUseSingleClipFallback && animationClips.size() == 1) {
        return &animationClips.front();
    }

    if (requestedName.empty()) {
        return nullptr;
    }

    for (const AnimationClip& animationClip : animationClips) {
        if (animationClip.name == requestedName) {
            return &animationClip;
        }
    }

    const std::string normalizedRequestedName = NormalizeAnimationName(requestedName);
    for (const AnimationClip& animationClip : animationClips) {
        const std::string normalizedClipName = NormalizeAnimationName(animationClip.name);
        if (normalizedClipName == normalizedRequestedName ||
            GetAnimationNameSuffix(normalizedClipName) == normalizedRequestedName) {
            return &animationClip;
        }
    }

    const AnimationClip* partialMatch = nullptr;
    for (const AnimationClip& animationClip : animationClips) {
        const std::string normalizedClipName = NormalizeAnimationName(animationClip.name);
        if (normalizedClipName.find(normalizedRequestedName) == std::string::npos) {
            continue;
        }

        if (partialMatch) {
            return nullptr;
        }
        partialMatch = &animationClip;
    }

    return partialMatch;
}
