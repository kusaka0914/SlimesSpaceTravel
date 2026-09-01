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

class PlayerAnimationController {
public:
    void Configure(PlayerAnimationDefinitions animationDefinitions);
    void SetLoadedModel(const LoadedModel* loadedModel);



    bool RequestAnimation(std::string_view animationId, bool shouldRestart = true);
    bool HasAnimation(std::string_view animationId) const;

    void Update(float deltaTimeSeconds);
    void ResetToAnimation(std::string_view animationId = "idle");

    const std::vector<glm::mat4>* GetSkinningMatrices() const;

private:
    struct ResolvedAnimation {
        const AnimationClip* clip = nullptr;
        PlayerAnimationPlaybackMode playbackMode = PlayerAnimationPlaybackMode::OneShot;
    };

    void ResolveAnimationClips();
    void PlayBaseAnimation();
    void PlayResolvedAnimation(const std::string& animationId, const ResolvedAnimation& animation,
                               bool shouldRestart);

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
