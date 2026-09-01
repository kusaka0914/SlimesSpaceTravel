#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

struct AnimationPositionKey {
    glm::vec3 position{0.0f};
    double timeTicks = 0.0;
};

struct AnimationRotationKey {
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    double timeTicks = 0.0;
};

struct AnimationScaleKey {
    glm::vec3 scale{1.0f};
    double timeTicks = 0.0;
};

struct BoneAnimationChannel {
    std::string nodeName;
    std::vector<AnimationPositionKey> positionKeys;
    std::vector<AnimationRotationKey> rotationKeys;
    std::vector<AnimationScaleKey> scaleKeys;
};

struct AnimationClip {
    std::string name;
    double durationTicks = 0.0;
    double ticksPerSecond = 25.0;
    std::unordered_map<std::string, BoneAnimationChannel> channelsByNodeName;
};

struct SkeletonNode {
    std::string name;
    glm::mat4 localBindTransform{1.0f};
    glm::vec3 bindPosition{0.0f};
    glm::quat bindRotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 bindScale{1.0f};
    std::vector<SkeletonNode> children;
};

struct BoneInfo {
    std::string name;
    glm::mat4 inverseBindTransform{1.0f};
};

struct SkeletalAnimationData {
    SkeletonNode rootNode;
    glm::mat4 inverseRootTransform{1.0f};
    std::vector<BoneInfo> bones;
    std::unordered_map<std::string, std::size_t> boneIndexByName;
    std::vector<AnimationClip> clips;

    bool HasSkinningData() const { return !bones.empty(); }
    bool HasAnimationClips() const { return !clips.empty(); }
};
