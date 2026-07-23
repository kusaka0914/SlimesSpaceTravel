#include "actor/player/PlayerAnimationController.h"

#include "system/mesh/LoadedModel.h"

#include <cctype>
#include <iostream>
#include <string>
#include <utility>

namespace {
std::string NormalizeAnimationId(std::string_view animationId)
{
    std::string normalizedId;
    normalizedId.reserve(animationId.size());

    for (const unsigned char character : animationId) {
        if (std::isspace(character) || character == '-' || character == '_') {
            continue;
        }
        normalizedId.push_back(static_cast<char>(std::tolower(character)));
    }

    return normalizedId;
}

std::string NormalizeClipName(const std::string& animationName)
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

void PlayerAnimationController::Configure(PlayerAnimationDefinitions animationDefinitions)
{
    mAnimationDefinitions.clear();
    for (auto& [animationId, definition] : animationDefinitions) {
        const std::string normalizedId = NormalizeAnimationId(animationId);
        if (!normalizedId.empty()) {
            mAnimationDefinitions[normalizedId] = std::move(definition);
        }
    }

    if (mLoadedModel) {
        ResolveAnimationClips();
        ResetToAnimation(mBaseAnimationId);
    }
}

void PlayerAnimationController::SetLoadedModel(const LoadedModel* loadedModel)
{
    mLoadedModel = loadedModel;

    const SkeletalAnimationData* animationData =
        mLoadedModel && mLoadedModel->HasSkinningData() ? &mLoadedModel->skeletalAnimation : nullptr;
    mAnimationPlayer.SetAnimationData(animationData);

    ResolveAnimationClips();
    ResetToAnimation(mBaseAnimationId);
}

bool PlayerAnimationController::RequestAnimation(std::string_view animationId, bool shouldRestart)
{
    const std::string normalizedId = NormalizeAnimationId(animationId);
    const ResolvedAnimation* animation = FindResolvedAnimation(normalizedId);
    if (!animation || !animation->clip) {
        return false;
    }

    if (animation->playbackMode == PlayerAnimationPlaybackMode::BaseLoop) {
        mBaseAnimationId = normalizedId;

        if (!mIsOneShotPlaying) {
            PlayResolvedAnimation(normalizedId, *animation, false);
        }
        return true;
    }

    PlayResolvedAnimation(normalizedId, *animation, shouldRestart);
    mIsOneShotPlaying = true;
    return true;
}

bool PlayerAnimationController::HasAnimation(std::string_view animationId) const
{
    const ResolvedAnimation* animation = FindResolvedAnimation(animationId);
    return animation && animation->clip;
}

void PlayerAnimationController::Update(float deltaTimeSeconds)
{
    if (!mAnimationPlayer.HasSkinningData()) {
        return;
    }

    mAnimationPlayer.Update(deltaTimeSeconds);

    if (!mIsOneShotPlaying || !mAnimationPlayer.IsFinished()) {
        return;
    }

    mIsOneShotPlaying = false;
    PlayBaseAnimation();
}

void PlayerAnimationController::ResetToAnimation(std::string_view animationId)
{
    mBaseAnimationId = NormalizeAnimationId(animationId);
    mCurrentAnimationId.clear();
    mIsOneShotPlaying = false;

    if (!mAnimationPlayer.HasSkinningData()) {
        return;
    }

    PlayBaseAnimation();
}

const std::vector<glm::mat4>* PlayerAnimationController::GetSkinningMatrices() const
{
    return mAnimationPlayer.HasSkinningData() ? &mAnimationPlayer.GetFinalBoneTransforms() : nullptr;
}

void PlayerAnimationController::ResolveAnimationClips()
{
    mResolvedAnimations.clear();

    if (!mLoadedModel || !mLoadedModel->HasAnimationClips()) {
        return;
    }

    for (const auto& [animationId, definition] : mAnimationDefinitions) {
        const AnimationClip* clip = FindAnimationClip(definition.clipName);
        mResolvedAnimations[animationId] = {clip, definition.playbackMode};

        if (!clip) {
            std::cerr << "Player animation '" << animationId << "' could not find clip '" << definition.clipName
                      << "'.\n";
        }
    }

    bool hasMissingClip = false;
    for (const auto& [animationId, animation] : mResolvedAnimations) {
        if (!animation.clip) {
            hasMissingClip = true;
            break;
        }
    }

    if (!hasMissingClip) {
        return;
    }

    std::cerr << "Available animation clips:";
    for (const AnimationClip& animationClip : mLoadedModel->skeletalAnimation.clips) {
        std::cerr << " '" << animationClip.name << "'";
    }
    std::cerr << '\n';
}

void PlayerAnimationController::PlayBaseAnimation()
{
    const ResolvedAnimation* baseAnimation = FindResolvedAnimation(mBaseAnimationId);
    if (!baseAnimation || !baseAnimation->clip) {
        mCurrentAnimationId.clear();
        mAnimationPlayer.ResetToBindPose();
        return;
    }

    PlayResolvedAnimation(mBaseAnimationId, *baseAnimation, false);
}

void PlayerAnimationController::PlayResolvedAnimation(const std::string& animationId,
                                                       const ResolvedAnimation& animation,
                                                       bool shouldRestart)
{
    if (!animation.clip) {
        return;
    }

    const bool isSameAnimation = mCurrentAnimationId == animationId && mAnimationPlayer.IsPlaying(animation.clip);
    if (isSameAnimation && !shouldRestart) {
        return;
    }

    const bool shouldLoop = animation.playbackMode == PlayerAnimationPlaybackMode::BaseLoop;
    mAnimationPlayer.Play(animation.clip, shouldLoop, shouldRestart);
    mCurrentAnimationId = animationId;
}

const PlayerAnimationController::ResolvedAnimation*
PlayerAnimationController::FindResolvedAnimation(std::string_view animationId) const
{
    const std::string normalizedId = NormalizeAnimationId(animationId);
    const auto animationIt = mResolvedAnimations.find(normalizedId);
    return animationIt == mResolvedAnimations.end() ? nullptr : &animationIt->second;
}

const AnimationClip* PlayerAnimationController::FindAnimationClip(const std::string& requestedName) const
{
    if (!mLoadedModel || !mLoadedModel->HasAnimationClips() || requestedName.empty()) {
        return nullptr;
    }

    const std::vector<AnimationClip>& animationClips = mLoadedModel->skeletalAnimation.clips;

    for (const AnimationClip& animationClip : animationClips) {
        if (animationClip.name == requestedName) {
            return &animationClip;
        }
    }

    const std::string normalizedRequestedName = NormalizeClipName(requestedName);
    for (const AnimationClip& animationClip : animationClips) {
        const std::string normalizedClipName = NormalizeClipName(animationClip.name);
        if (normalizedClipName == normalizedRequestedName ||
            GetAnimationNameSuffix(normalizedClipName) == normalizedRequestedName) {
            return &animationClip;
        }
    }

    const AnimationClip* partialMatch = nullptr;
    for (const AnimationClip& animationClip : animationClips) {
        const std::string normalizedClipName = NormalizeClipName(animationClip.name);
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
