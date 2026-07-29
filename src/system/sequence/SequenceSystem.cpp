#include "system/sequence/SequenceSystem.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "system/CameraSystem.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float EventStartEpsilon = 0.0001f;
}

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

    if (mActiveSequence->loop && duration > EventStartEpsilon && mElapsedTime > duration) {
        ApplyMovementClips(*mActiveSequence, duration);
        ApplyEventClips(*mActiveSequence, previousTime, duration);

        mElapsedTime = std::fmod(mElapsedTime, duration);
        ApplyEventClips(*mActiveSequence, -EventStartEpsilon, mElapsedTime);
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

bool SequenceSystem::Play(const std::string& sequenceId, bool preview)
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
    mIsPreview = preview;
    mLocksPlayerControl = false;
    mLastError.clear();

    if (preview) {
        CapturePreviewState(*sequence);
    }

    ApplyEventClips(*sequence, -EventStartEpsilon, 0.0f);
    ApplyMovementClips(*sequence, 0.0f);
    return true;
}

bool SequenceSystem::PlayCinematicChainThenSequence(
    const std::vector<std::string>& cinematicSequenceIds,
    const std::string& gameplaySequenceId)
{
    Stop(true);

    if (!mGame || !mGame->GetCameraSystem() || cinematicSequenceIds.empty() || gameplaySequenceId.empty()) {
        mLastError = "Invalid cinematic chain";
        return false;
    }

    if (!mLibrary.Find(gameplaySequenceId)) {
        mLastError = "Sequence not found: " + gameplaySequenceId;
        return false;
    }

    for (const std::string& cinematicSequenceId : cinematicSequenceIds) {
        if (cinematicSequenceId.empty() ||
            !mGame->GetCameraSystem()->GetCinematicLibrary().Find(cinematicSequenceId)) {
            mLastError = "Camera sequence not found: " + cinematicSequenceId;
            return false;
        }
    }

    mCinematicSequenceChain = cinematicSequenceIds;
    mCinematicSequenceIndex = 0;
    mGameplaySequenceAfterCinematics = gameplaySequenceId;
    mIsCinematicChainPlaying = true;
    mLocksPlayerControl = true;
    mActiveSequenceId = gameplaySequenceId;
    mLastError.clear();

    if (!mGame->GetCameraSystem()->PlayCinematic(mCinematicSequenceChain.front())) {
        mLastError = "Failed to play camera sequence: " + mCinematicSequenceChain.front();
        ClearCinematicChain();
        mLocksPlayerControl = false;
        mActiveSequenceId.clear();
        return false;
    }

    return true;
}

void SequenceSystem::Stop(bool restorePreviewState)
{
    if (mGame && mGame->GetCameraSystem() && (mIsPreview || mIsCinematicChainPlaying)) {
        mGame->GetCameraSystem()->StopCinematic();
    }

    if (restorePreviewState) {
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

    StageActorRef stageRef;
    stageRef.sequenceName = actorRef.group;
    stageRef.yamlIndex = actorRef.index;
    return StageActorQuery::FindActorByRef(stage, stageRef);
}

void SequenceSystem::CapturePreviewState(const GameplaySequence& sequence)
{
    mPreviewSnapshots.clear();

    for (const SequenceClip& clip : sequence.clips) {
        if (clip.type != SequenceClipType::ActorMove &&
            clip.type != SequenceClipType::ActorVisibility) {
            continue;
        }

        Actor* actor = ResolveActor(clip.actor);
        if (!actor || mPreviewSnapshots.contains(actor)) {
            continue;
        }

        mPreviewSnapshots[actor] = {actor->GetPos(), actor->GetIsActive()};
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
                mLastError =
                    "Actor not found: " + clip.actor.group + ":" + std::to_string(clip.actor.index);
            }
            continue;
        }

        const float duration = std::max(clip.duration, 0.001f);
        const float linearT = std::clamp((time - clip.startTime) / duration, 0.0f, 1.0f);
        const float easedT = ApplyEasing(linearT, clip.easing);
        actor->SetPos(glm::mix(clip.fromPosition, clip.toPosition, easedT));
    }
}

void SequenceSystem::ApplyEventClips(
    const GameplaySequence& sequence,
    float previousTime,
    float time)
{
    for (const SequenceClip& clip : sequence.clips) {
        if (clip.type == SequenceClipType::ActorMove) {
            continue;
        }

        if (clip.startTime > previousTime && clip.startTime <= time + EventStartEpsilon) {
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
            mLastError =
                "Actor not found: " + clip.actor.group + ":" + std::to_string(clip.actor.index);
        }
        break;
    }
    case SequenceClipType::PlayerControl:
        mLocksPlayerControl = !clip.playerControlEnabled;
        break;
    case SequenceClipType::Camera:
        if (mGame && mGame->GetCameraSystem() && !clip.cameraSequenceId.empty()) {
            if (!mGame->GetCameraSystem()->PlayCinematic(clip.cameraSequenceId) &&
                mLastError.empty()) {
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
    if (cameraSystem->IsCinematicPlaying() || !cameraSystem->HasCinematicFinished()) {
        return;
    }

    ++mCinematicSequenceIndex;
    if (mCinematicSequenceIndex < mCinematicSequenceChain.size()) {
        const std::string& nextCinematic = mCinematicSequenceChain[mCinematicSequenceIndex];
        if (!cameraSystem->PlayCinematic(nextCinematic)) {
            mLastError = "Failed to play camera sequence: " + nextCinematic;
            const std::string gameplaySequenceId = mGameplaySequenceAfterCinematics;
            ClearCinematicChain();
            Play(gameplaySequenceId);
        }
        return;
    }

    const std::string gameplaySequenceId = mGameplaySequenceAfterCinematics;
    ClearCinematicChain();
    Play(gameplaySequenceId);
}

void SequenceSystem::ClearCinematicChain()
{
    mCinematicSequenceChain.clear();
    mCinematicSequenceIndex = 0;
    mGameplaySequenceAfterCinematics.clear();
    mIsCinematicChainPlaying = false;
}

float SequenceSystem::ApplyEasing(float t, SequenceEasing easing)
{
    t = std::clamp(t, 0.0f, 1.0f);
    switch (easing) {
    case SequenceEasing::Linear:
        return t;
    case SequenceEasing::EaseIn:
        return t * t;
    case SequenceEasing::EaseOut:
        return 1.0f - (1.0f - t) * (1.0f - t);
    case SequenceEasing::EaseInOut:
    default:
        return t * t * (3.0f - 2.0f * t);
    }
}
