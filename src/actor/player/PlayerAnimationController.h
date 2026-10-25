#pragma once

#include "actor/player/PlayerAnimationDefinition.h"
#include "animation/AnimationPlayer.h"

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct AnimationClip;
struct LoadedModel;

// 論理的なアニメーションIDをモデルのクリップへ対応付け、単発再生と基本ループを管理する。
class PlayerAnimationController {
public:
    // 定義を置き換える。モデル設定済みならクリップを再解決し、基本ループへリセットする。
    void Configure(PlayerAnimationDefinitions animationDefinitions);

    // モデルは非所有参照。クリップと再生状態を再設定し、nullptrならスキニングを解除する。
    void SetLoadedModel(const LoadedModel* loadedModel);

    // 対応クリップがなければfalse。単発再生中の基本ループ要求は、単発終了後に反映する。
    // shouldRestartは単発再生の再要求時に、先頭から再生するかを指定する。
    bool RequestAnimation(std::string_view animationId, bool shouldRestart = true);
    bool HasAnimation(std::string_view animationId) const;

    void Update(float deltaTimeSeconds);

    // 単発再生を中断して指定IDへ戻す。クリップ未解決ならバインドポーズを使う。
    void ResetToAnimation(std::string_view animationId = "idle");

    // スキニングデータなしではnullptr。返すポインタは内部行列配列への非所有参照。
    const std::vector<glm::mat4>* GetSkinningMatrices() const;

private:
    struct ResolvedAnimation {
        const AnimationClip* clip = nullptr;
        PlayerAnimationPlaybackMode playbackMode = PlayerAnimationPlaybackMode::OneShot;
    };

    void ResolveAnimationClips();
    void PlayBaseAnimation();
    void PlayResolvedAnimation(const std::string& animationId, const ResolvedAnimation& animation, bool shouldRestart);

    const ResolvedAnimation* FindResolvedAnimation(std::string_view animationId) const;
    const AnimationClip* FindAnimationClip(const std::string& requestedName) const;

private:
    const LoadedModel* mLoadedModel = nullptr;

    PlayerAnimationDefinitions mAnimationDefinitions;
    std::unordered_map<std::string, ResolvedAnimation> mResolvedAnimations;

    std::string mBaseAnimationId = "idle";
    std::string mCurrentAnimationId;
    bool mIsOneShotPlaying = false;

    AnimationPlayer mAnimationPlayer;
};
