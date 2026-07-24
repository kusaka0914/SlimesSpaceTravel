#pragma once

#include "system/sequence/SequenceLibrary.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>

class Actor;
class Game;

class SequenceSystem {
public:
    explicit SequenceSystem(Game* game);

    void Update(float deltaTime);

    bool Play(const std::string& sequenceId, bool preview = false);
    void Stop(bool restorePreviewState = true);

    bool IsPlaying() const { return mIsPlaying; }
    bool IsPreviewing() const { return mIsPreview; }
    bool HasPreviewSnapshot() const { return !mPreviewSnapshots.empty(); }
    bool LocksPlayerControl() const { return mLocksPlayerControl; }

    float GetElapsedTime() const { return mElapsedTime; }
    float GetDuration() const;
    const std::string& GetActiveSequenceId() const { return mActiveSequenceId; }
    const std::string& GetLastError() const { return mLastError; }

    SequenceLibrary& GetLibrary() { return mLibrary; }
    const SequenceLibrary& GetLibrary() const { return mLibrary; }

    bool Save() const { return mLibrary.Save(); }
    bool Reload();

    Actor* ResolveActor(const SequenceActorRef& actorRef) const;

private:
    struct ActorSnapshot {
        glm::vec3 position{0.0f};
        bool active = true;
    };

    void CapturePreviewState(const GameplaySequence& sequence);
    void RestorePreviewState();

    void ApplyMovementClips(const GameplaySequence& sequence, float time);
    void ApplyEventClips(const GameplaySequence& sequence, float previousTime, float time);
    void ApplyEventClip(const SequenceClip& clip);

    static float ApplyEasing(float t, SequenceEasing easing);

private:
    Game* mGame = nullptr;
    SequenceLibrary mLibrary;

    const GameplaySequence* mActiveSequence = nullptr;
    std::string mActiveSequenceId;
    std::string mLastError;

    float mElapsedTime = 0.0f;
    bool mIsPlaying = false;
    bool mIsPreview = false;
    bool mLocksPlayerControl = false;

    std::unordered_map<Actor*, ActorSnapshot> mPreviewSnapshots;
};
