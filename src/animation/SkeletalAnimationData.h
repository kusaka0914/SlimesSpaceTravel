#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

// ノードのローカル変換を表すキー。各timeTicksはクリップ内の時刻で、秒への換算にはticksPerSecondを使う。
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

// 1ノードの位置・回転・スケールのキー列。各配列はtimeTicksの昇順に並べ、回転には有効なクォータニオンを使う。
// キーのない成分は、そのノードのバインド姿勢を維持する。
struct BoneAnimationChannel {
    std::string nodeName;
    std::vector<AnimationPositionKey> positionKeys;
    std::vector<AnimationRotationKey> rotationKeys;
    std::vector<AnimationScaleKey> scaleKeys;
};

// ノード名で参照するアニメーション。期間・キー時刻はtick単位で、ticksPerSecondが非正なら再生時に25を使う。
struct AnimationClip {
    std::string name;
    double durationTicks = 0.0;
    double ticksPerSecond = 25.0;
    std::unordered_map<std::string, BoneAnimationChannel> channelsByNodeName;
};

// ボーン以外も含むノード階層。localBindTransformは親に対するバインド変換、bind…はその分解値。
struct SkeletonNode {
    std::string name;
    glm::mat4 localBindTransform{1.0f};
    glm::vec3 bindPosition{0.0f};
    glm::quat bindRotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 bindScale{1.0f};
    std::vector<SkeletonNode> children;
};

// スキニング対象のボーン。inverseBindTransformはメッシュのバインド座標からボーンのバインド座標への変換。
struct BoneInfo {
    std::string name;
    glm::mat4 inverseBindTransform{1.0f};
};

// モデルが所有するスケルトン・ボーン・クリップ。boneIndexByNameの値はbonesと描画用行列配列の添字に対応する。
struct SkeletalAnimationData {
    SkeletonNode rootNode;
    glm::mat4 inverseRootTransform{1.0f};
    std::vector<BoneInfo> bones;
    std::unordered_map<std::string, std::size_t> boneIndexByName;
    std::vector<AnimationClip> clips;

    bool HasSkinningData() const { return !bones.empty(); }
    bool HasAnimationClips() const { return !clips.empty(); }
};
