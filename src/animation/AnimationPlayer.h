#pragma once

#include "animation/SkeletalAnimationData.h"

#include <glm/glm.hpp>
#include <vector>

// スケルトンのクリップ再生とキーフレーム補間を行い、描画用のボーン変換行列を計算する。
// データとクリップは非所有参照。設定を解除・変更するまで参照先を有効に保つこと。
class AnimationPlayer {
public:
    // データを切り替え、再生状態を初期化してバインドポーズを計算する。nullptrなら解除して行列配列を空にする。
    void SetAnimationData(const SkeletalAnimationData* animationData);

    // 設定データと同じスケルトン用のクリップを指定する。データ未設定・nullptrなら何もしない。
    // 同じクリップでshouldRestartがfalseなら、再生時刻と完了状態を保ち、ループ設定だけを変更する。
    void Play(const AnimationClip* animationClip, bool shouldLoop, bool shouldRestart = true);

    // クリップ再生と完了状態を解除し、設定データのバインドポーズへ戻す。
    void ResetToBindPose();

    // 秒数をクリップのtickへ変換して進める。単発再生は終端を保持して完了状態にし、ループ再生は先頭へ折り返す。
    // 非正の秒数・データ未設定・クリップ未選択なら何もしない。
    void Update(float deltaTimeSeconds);

    bool HasSkinningData() const;
    bool IsFinished() const { return mIsFinished; }
    bool IsPlaying(const AnimationClip* animationClip) const { return mCurrentClip == animationClip; }

    const AnimationClip* GetCurrentClip() const { return mCurrentClip; }

    // SkeletalAnimationData::bonesと同じ順序のスキニング行列。データ未設定なら空配列。
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
