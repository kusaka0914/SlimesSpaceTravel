#include "system/sequence/SequenceSystem.h"

#include "app/Game.h"
#include "world/Stage.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "system/AudioSystem.h"
#include "system/CameraSystem.h"
#include "system/SceneSystem.h"
#include "system/actor_loader/StageActorLocator.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float eventStartEpsilon = 0.0001f;
} // namespace

SequenceSystem::SequenceSystem(Game* game)
    : mGame(game),
      mLibrary("../assets/data/sequences/sequences.yaml")
{
    mLibrary.Load();
}

void SequenceSystem::Update(float deltaTime)
{
    if (mIsCinematicChainPlaying) {
        UpdateCinematicChain();
        return;
    }

    if (!mIsPlaying || !mActiveSequence) {
        return;
    }

    const float duration = mActiveSequence->CalculateDuration();
    const float previousTime = mElapsedTime;
    mElapsedTime += std::max(0.0f, deltaTime);

    if (mActiveSequence->loop && duration > eventStartEpsilon && mElapsedTime > duration) {
        ApplyMovementClips(*mActiveSequence, duration);
        ApplyEventClips(*mActiveSequence, previousTime, duration);

        mElapsedTime = std::fmod(mElapsedTime, duration);
        ApplyEventClips(*mActiveSequence, -eventStartEpsilon, mElapsedTime);
        ApplyMovementClips(*mActiveSequence, mElapsedTime);
        return;
    }

    const float evaluatedTime = duration > 0.0f ? std::min(mElapsedTime, duration) : mElapsedTime;
    ApplyEventClips(*mActiveSequence, previousTime, evaluatedTime);
    ApplyMovementClips(*mActiveSequence, evaluatedTime);

    if (mElapsedTime >= duration) {
        mElapsedTime = duration;
        mIsPlaying = false;
        mLocksPlayerControl = false;
    }
}

bool SequenceSystem::Play(const std::string& sequenceId, bool isPreview)
{
    Stop(true);

    const GameplaySequence* sequence = mLibrary.Find(sequenceId);
    if (!sequence) {
        mLastError = "Sequence not found: " + sequenceId;
        return false;
    }

    mActiveSequence = sequence;
    mActiveSequenceId = sequenceId;
    mElapsedTime = 0.0f;
    mIsPlaying = true;
    mIsPreview = isPreview;
    mLocksPlayerControl = false;
    mLastError.clear();

    if (isPreview) {
        CapturePreviewState(*sequence);
    }

    ApplyEventClips(*sequence, -eventStartEpsilon, 0.0f);
    ApplyMovementClips(*sequence, 0.0f);
    return true;
}

bool SequenceSystem::PlayCinematicChainThenSequence(const std::vector<std::string>& cinematicSequenceIds,
                                                    const std::string& gameplaySequenceId)
{
    if (gameplaySequenceId.empty() || !mLibrary.Find(gameplaySequenceId)) {
        mLastError = "Sequence not found: " + gameplaySequenceId;
        return false;
    }

    return StartCinematicChain(cinematicSequenceIds, gameplaySequenceId);
}

bool SequenceSystem::PlayCinematicChain(const std::vector<std::string>& cinematicSequenceIds)
{
    return StartCinematicChain(cinematicSequenceIds, {});
}

bool SequenceSystem::StartCinematicChain(const std::vector<std::string>& cinematicSequenceIds,
                                         const std::string& gameplaySequenceId)
{
    Stop(true);

    if (!mGame || !mGame->GetCameraSystem() || cinematicSequenceIds.empty()) {
        mLastError = "Invalid cinematic chain";
        return false;
    }

    for (const std::string& cinematicSequenceId : cinematicSequenceIds) {
        if (cinematicSequenceId.empty() || !mGame->GetCameraSystem()->GetCinematicLibrary().Find(cinematicSequenceId)) {
            mLastError = "Camera sequence not found: " + cinematicSequenceId;
            return false;
        }
    }

    mCinematicSequenceChain = cinematicSequenceIds;
    mCinematicSequenceIndex = 0;
    mGameplaySequenceAfterCinematics = gameplaySequenceId;
    mIsCinematicChainPlaying = true;
    mIsCinematicReturnFadeRequested = false;
    mLocksPlayerControl = true;
    mActiveSequenceId = gameplaySequenceId.empty() ? cinematicSequenceIds.front() : gameplaySequenceId;
    mLastError.clear();

    if (!mGame->GetCameraSystem()->PlayCinematic(mCinematicSequenceChain.front(), true)) {
        mLastError = "カメラ演出を再生できませんでした: " + mCinematicSequenceChain.front();
        ClearCinematicChain();
        mLocksPlayerControl = false;
        mActiveSequenceId.clear();
        return false;
    }

    return true;
}

void SequenceSystem::Stop(bool shouldRestorePreviewState)
{
    ++mCinematicChainGeneration;

    if (mGame && mGame->GetCameraSystem() && (mIsPreview || mIsCinematicChainPlaying)) {
        mGame->GetCameraSystem()->StopCinematic();
    }

    if (shouldRestorePreviewState) {
        RestorePreviewState();
    } else {
        mPreviewSnapshots.clear();
    }

    mActiveSequence = nullptr;
    mActiveSequenceId.clear();
    mElapsedTime = 0.0f;
    mIsPlaying = false;
    mIsPreview = false;
    mLocksPlayerControl = false;
    ClearCinematicChain();
}

float SequenceSystem::GetDuration() const
{
    return mActiveSequence ? mActiveSequence->CalculateDuration() : 0.0f;
}

bool SequenceSystem::Reload()
{
    Stop(true);
    return mLibrary.Load();
}

Actor* SequenceSystem::ResolveActor(const SequenceActorRef& actorRef) const
{
    if (!mGame || !actorRef.IsValid()) {
        return nullptr;
    }

    if (actorRef.group == "players") {
        const auto& players = mGame->GetPlayers();
        if (actorRef.index >= 0 && actorRef.index < static_cast<int>(players.size())) {
            return players[actorRef.index];
        }
        return nullptr;
    }

    Stage* stage = mGame->GetCurrentStage();
    if (!stage) {
        return nullptr;
    }

    return StageActorLocator{}.FindPlacedActor(*stage, actorRef.group, actorRef.index);
}

void SequenceSystem::CapturePreviewState(const GameplaySequence& sequence)
{
    mPreviewSnapshots.clear();

    for (const SequenceClip& clip : sequence.clips) {
        if (clip.type != SequenceClipType::ActorMove && clip.type != SequenceClipType::ActorVisibility) {
            continue;
        }

        Actor* actor = ResolveActor(clip.actor);
        if (!actor || mPreviewSnapshots.contains(actor)) {
            continue;
        }

        mPreviewSnapshots[actor] = {actor->GetPos(), actor->IsExplicitlyActive()};
    }
}

void SequenceSystem::RestorePreviewState()
{
    for (const auto& [actor, snapshot] : mPreviewSnapshots) {
        if (!actor) {
            continue;
        }
        actor->SetPos(snapshot.position);
        actor->SetIsActive(snapshot.active);
    }
    mPreviewSnapshots.clear();
}

void SequenceSystem::ApplyMovementClips(const GameplaySequence& sequence, float time)
{
    for (const SequenceClip& clip : sequence.clips) {
        if (clip.type != SequenceClipType::ActorMove || time < clip.startTime) {
            continue;
        }

        Actor* actor = ResolveActor(clip.actor);
        if (!actor) {
            if (mLastError.empty()) {
                mLastError = "Actor not found: " + clip.actor.group + ":" + std::to_string(clip.actor.index);
            }
            continue;
        }

        const float duration = std::max(clip.duration, 0.001f);
        const float linearProgress = std::clamp((time - clip.startTime) / duration, 0.0f, 1.0f);
        const float easedProgress = ApplyEasing(linearProgress, clip.easing);
        actor->SetPos(glm::mix(clip.fromPosition, clip.toPosition, easedProgress));
    }
}

void SequenceSystem::ApplyEventClips(const GameplaySequence& sequence, float previousTime, float time)
{
    for (const SequenceClip& clip : sequence.clips) {
        if (clip.type == SequenceClipType::ActorMove) {
            continue;
        }

        if (clip.startTime > previousTime && clip.startTime <= time + eventStartEpsilon) {
            ApplyEventClip(clip);
        }
    }
}

void SequenceSystem::ApplyEventClip(const SequenceClip& clip)
{
    switch (clip.type) {
    case SequenceClipType::ActorVisibility: {
        Actor* actor = ResolveActor(clip.actor);
        if (actor) {
            actor->SetIsActive(clip.visible);
        } else if (mLastError.empty()) {
            mLastError = "Actor not found: " + clip.actor.group + ":" + std::to_string(clip.actor.index);
        }
        break;
    }
    case SequenceClipType::PlayerControl:
        mLocksPlayerControl = !clip.playerControlEnabled;
        break;
    case SequenceClipType::Camera:
        if (mGame && mGame->GetCameraSystem() && !clip.cameraSequenceId.empty()) {
            if (!mGame->GetCameraSystem()->PlayCinematic(clip.cameraSequenceId) && mLastError.empty()) {
                mLastError = "Camera sequence not found: " + clip.cameraSequenceId;
            }
        }
        break;
    case SequenceClipType::ActorMove:
    default:
        break;
    }
}

void SequenceSystem::UpdateCinematicChain()
{
    if (!mGame || !mGame->GetCameraSystem()) {
        ClearCinematicChain();
        mLocksPlayerControl = false;
        return;
    }

    CameraSystem* cameraSystem = mGame->GetCameraSystem();
    if (!cameraSystem->HasCinematicFinished()) {
        return;
    }

    if (mIsCinematicReturnFadeRequested) {
        return;
    }

    ++mCinematicSequenceIndex;
    if (mCinematicSequenceIndex < mCinematicSequenceChain.size()) {
        const std::string& nextCinematic = mCinematicSequenceChain[mCinematicSequenceIndex];
        if (!cameraSystem->PlayCinematic(nextCinematic, true)) {
            mLastError = "カメラ演出を再生できませんでした: " + nextCinematic;
            RequestCinematicReturnFade();
        }
        return;
    }

    RequestCinematicReturnFade();
}

void SequenceSystem::RequestCinematicReturnFade()
{
    if (mIsCinematicReturnFadeRequested || !mGame) {
        return;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (!sceneSystem) {
        FinishCinematicChainAtFadeMidpoint(mCinematicChainGeneration);
        return;
    }

    const std::size_t cinematicChainGeneration = mCinematicChainGeneration;
    const bool requested = sceneSystem->RequestFadeAction(
        [this, cinematicChainGeneration]() { FinishCinematicChainAtFadeMidpoint(cinematicChainGeneration); },
        [game = mGame]() {
            if (game && game->GetAudioSystem()) {
                game->GetAudioSystem()->ResumeDeferredStageMusic();
            }
        });
    if (requested) {
        mIsCinematicReturnFadeRequested = true;
    }
}

void SequenceSystem::FinishCinematicChainAtFadeMidpoint(std::size_t cinematicChainGeneration)
{
    // 停止や別の再生要求の後に、古いフェードのコールバックが状態を変更するのを防ぐ。
    if (cinematicChainGeneration != mCinematicChainGeneration || !mIsCinematicChainPlaying) {
        return;
    }

    if (mGame && mGame->GetCameraSystem()) {
        mGame->GetCameraSystem()->StopCinematic();
        mGame->GetCameraSystem()->SnapBehindControlledPlayer();
    }

    const std::string gameplaySequenceId = mGameplaySequenceAfterCinematics;
    ClearCinematicChain();
    mLocksPlayerControl = false;
    mActiveSequenceId.clear();

    if (!gameplaySequenceId.empty()) {
        Play(gameplaySequenceId);
    }
}

void SequenceSystem::ClearCinematicChain()
{
    mCinematicSequenceChain.clear();
    mCinematicSequenceIndex = 0;
    mGameplaySequenceAfterCinematics.clear();
    mIsCinematicChainPlaying = false;
    mIsCinematicReturnFadeRequested = false;
}

float SequenceSystem::ApplyEasing(float progress, SequenceEasing easing)
{
    progress = std::clamp(progress, 0.0f, 1.0f);
    switch (easing) {
    case SequenceEasing::Linear:
        return progress;
    case SequenceEasing::EaseIn:
        return progress * progress;
    case SequenceEasing::EaseOut:
        return 1.0f - (1.0f - progress) * (1.0f - progress);
    case SequenceEasing::EaseInOut:
    default:
        return progress * progress * (3.0f - 2.0f * progress);
    }
}
