#include "animation/AnimationPlayer.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace {
template <class KeyType>
std::size_t FindKeyInterval(const std::vector<KeyType>& keys, double currentTimeTicks)
{
    if (keys.size() < 2) {
        return 0;
    }

    for (std::size_t keyIndex = 0; keyIndex + 1 < keys.size(); ++keyIndex) {
        if (currentTimeTicks < keys[keyIndex + 1].timeTicks) {
            return keyIndex;
        }
    }

    return keys.size() - 2;
}

template <class KeyType>
float CalculateInterpolationFactor(const KeyType& currentKey, const KeyType& nextKey, double currentTimeTicks)
{
    const double intervalTicks = nextKey.timeTicks - currentKey.timeTicks;
    if (intervalTicks <= 0.0) {
        return 0.0f;
    }

    const double elapsedTicks = currentTimeTicks - currentKey.timeTicks;
    return glm::clamp(static_cast<float>(elapsedTicks / intervalTicks), 0.0f, 1.0f);
}
}

void AnimationPlayer::SetAnimationData(const SkeletalAnimationData* animationData)
{
    mAnimationData = animationData;
    mCurrentClip = nullptr;
    mCurrentTimeTicks = 0.0;
    mShouldLoop = false;
    mIsFinished = false;

    const std::size_t boneCount = mAnimationData ? mAnimationData->bones.size() : 0;
    mFinalBoneTransforms.assign(boneCount, glm::mat4(1.0f));

    ResetToBindPose();
}

void AnimationPlayer::Play(const AnimationClip* animationClip, bool shouldLoop, bool shouldRestart)
{
    if (!mAnimationData || !animationClip) {
        return;
    }

    const bool isSameClip = mCurrentClip == animationClip;
    if (isSameClip && !shouldRestart) {
        mShouldLoop = shouldLoop;
        return;
    }

    mCurrentClip = animationClip;
    mCurrentTimeTicks = 0.0;
    mShouldLoop = shouldLoop;
    mIsFinished = false;
    CalculateCurrentPose();
}

void AnimationPlayer::ResetToBindPose()
{
    mCurrentClip = nullptr;
    mCurrentTimeTicks = 0.0;
    mShouldLoop = false;
    mIsFinished = false;

    if (!mAnimationData) {
        return;
    }

    CalculateCurrentPose();
}

void AnimationPlayer::Update(float deltaTimeSeconds)
{
    if (!mAnimationData || !mCurrentClip || deltaTimeSeconds <= 0.0f) {
        return;
    }

    const double ticksPerSecond = mCurrentClip->ticksPerSecond > 0.0 ? mCurrentClip->ticksPerSecond : 25.0;
    const double durationTicks = mCurrentClip->durationTicks;

    if (durationTicks <= 0.0) {
        mCurrentTimeTicks = 0.0;
        mIsFinished = !mShouldLoop;
        CalculateCurrentPose();
        return;
    }

    mCurrentTimeTicks += static_cast<double>(deltaTimeSeconds) * ticksPerSecond;

    if (mShouldLoop) {
        mCurrentTimeTicks = std::fmod(mCurrentTimeTicks, durationTicks);
    } else if (mCurrentTimeTicks >= durationTicks) {
        mCurrentTimeTicks = durationTicks;
        mIsFinished = true;
    }

    CalculateCurrentPose();
}

bool AnimationPlayer::HasSkinningData() const
{
    return mAnimationData && mAnimationData->HasSkinningData() && !mFinalBoneTransforms.empty();
}

void AnimationPlayer::CalculateCurrentPose()
{
    if (!mAnimationData) {
        return;
    }

    CalculateNodeTransform(mAnimationData->rootNode, glm::mat4(1.0f));
}

void AnimationPlayer::CalculateNodeTransform(const SkeletonNode& node, const glm::mat4& parentTransform)
{
    const glm::mat4 localTransform = CalculateLocalTransform(node);
    const glm::mat4 globalTransform = parentTransform * localTransform;

    const auto boneIndexIt = mAnimationData->boneIndexByName.find(node.name);
    if (boneIndexIt != mAnimationData->boneIndexByName.end()) {
        const std::size_t boneIndex = boneIndexIt->second;
        if (boneIndex < mAnimationData->bones.size() && boneIndex < mFinalBoneTransforms.size()) {
            const BoneInfo& bone = mAnimationData->bones[boneIndex];
            mFinalBoneTransforms[boneIndex] =
                mAnimationData->inverseRootTransform * globalTransform * bone.inverseBindTransform;
        }
    }

    for (const SkeletonNode& child : node.children) {
        CalculateNodeTransform(child, globalTransform);
    }
}

glm::mat4 AnimationPlayer::CalculateLocalTransform(const SkeletonNode& node) const
{
    if (!mCurrentClip) {
        return node.localBindTransform;
    }

    const auto channelIt = mCurrentClip->channelsByNodeName.find(node.name);
    if (channelIt == mCurrentClip->channelsByNodeName.end()) {
        return node.localBindTransform;
    }

    const BoneAnimationChannel& channel = channelIt->second;
    const glm::vec3 position = InterpolatePosition(channel, node.bindPosition);
    const glm::quat rotation = InterpolateRotation(channel, node.bindRotation);
    const glm::vec3 scale = InterpolateScale(channel, node.bindScale);

    return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation) *
           glm::scale(glm::mat4(1.0f), scale);
}

glm::vec3 AnimationPlayer::InterpolatePosition(const BoneAnimationChannel& channel,
                                                const glm::vec3& fallbackPosition) const
{
    if (channel.positionKeys.empty()) {
        return fallbackPosition;
    }

    if (channel.positionKeys.size() == 1) {
        return channel.positionKeys.front().position;
    }

    const std::size_t keyIndex = FindKeyInterval(channel.positionKeys, mCurrentTimeTicks);
    const AnimationPositionKey& currentKey = channel.positionKeys[keyIndex];
    const AnimationPositionKey& nextKey = channel.positionKeys[keyIndex + 1];
    const float interpolationFactor = CalculateInterpolationFactor(currentKey, nextKey, mCurrentTimeTicks);

    return glm::mix(currentKey.position, nextKey.position, interpolationFactor);
}

glm::quat AnimationPlayer::InterpolateRotation(const BoneAnimationChannel& channel,
                                                const glm::quat& fallbackRotation) const
{
    if (channel.rotationKeys.empty()) {
        return fallbackRotation;
    }

    if (channel.rotationKeys.size() == 1) {
        return glm::normalize(channel.rotationKeys.front().rotation);
    }

    const std::size_t keyIndex = FindKeyInterval(channel.rotationKeys, mCurrentTimeTicks);
    const AnimationRotationKey& currentKey = channel.rotationKeys[keyIndex];
    const AnimationRotationKey& nextKey = channel.rotationKeys[keyIndex + 1];
    const float interpolationFactor = CalculateInterpolationFactor(currentKey, nextKey, mCurrentTimeTicks);

    return glm::normalize(glm::slerp(currentKey.rotation, nextKey.rotation, interpolationFactor));
}

glm::vec3 AnimationPlayer::InterpolateScale(const BoneAnimationChannel& channel, const glm::vec3& fallbackScale) const
{
    if (channel.scaleKeys.empty()) {
        return fallbackScale;
    }

    if (channel.scaleKeys.size() == 1) {
        return channel.scaleKeys.front().scale;
    }

    const std::size_t keyIndex = FindKeyInterval(channel.scaleKeys, mCurrentTimeTicks);
    const AnimationScaleKey& currentKey = channel.scaleKeys[keyIndex];
    const AnimationScaleKey& nextKey = channel.scaleKeys[keyIndex + 1];
    const float interpolationFactor = CalculateInterpolationFactor(currentKey, nextKey, mCurrentTimeTicks);

    return glm::mix(currentKey.scale, nextKey.scale, interpolationFactor);
}
