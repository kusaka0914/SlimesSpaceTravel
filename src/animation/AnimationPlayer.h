#pragma once

#include "animation/SkeletalAnimationData.h"

#include <glm/glm.hpp>

#include <vector>

class AnimationPlayer {
public:
    void SetAnimationData(const SkeletalAnimationData* animationData);

    void Play(const AnimationClip* animationClip, bool shouldLoop, bool shouldRestart = true);
    void ResetToBindPose();
    void Update(float deltaTimeSeconds);

    bool HasSkinningData() const;
    bool IsFinished() const { return mIsFinished; }
    bool IsPlaying(const AnimationClip* animationClip) const { return mCurrentClip == animationClip; }

    const AnimationClip* GetCurrentClip() const { return mCurrentClip; }
    const std::vector<glm::mat4>& GetFinalBoneTransforms() const { return mFinalBoneTransforms; }

private:
    void CalculateCurrentPose();
    void CalculateNodeTransform(const SkeletonNode& node, const glm::mat4& parentTransform);

    glm::mat4 CalculateLocalTransform(const SkeletonNode& node) const;
    glm::vec3 InterpolatePosition(const BoneAnimationChannel& channel, const glm::vec3& fallbackPosition) const;
    glm::quat InterpolateRotation(const BoneAnimationChannel& channel, const glm::quat& fallbackRotation) const;
    glm::vec3 InterpolateScale(const BoneAnimationChannel& channel, const glm::vec3& fallbackScale) const;

private:
    const SkeletalAnimationData* mAnimationData = nullptr;
    const AnimationClip* mCurrentClip = nullptr;

    double mCurrentTimeTicks = 0.0;
    bool mShouldLoop = false;
    bool mIsFinished = false;

    std::vector<glm::mat4> mFinalBoneTransforms;
};
