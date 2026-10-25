#pragma once

#include "system/sequence/SequenceLibrary.h"

#include <glm/glm.hpp>

#include <string>
#include <unordered_map>
#include <vector>

class Actor;
class Game;

// ゲーム演出とカメラ演出の連続再生を実行し、プレビュー時のActor状態を退避・復元する。
class SequenceSystem {
public:
    explicit SequenceSystem(Game* game);

    void Update(float deltaTime);

    // 現在の演出を停止し、指定IDを再生する。未登録ならfalse。isPreview時は対象Actorの位置・表示を退避する。
    bool Play(const std::string& sequenceId, bool isPreview = false);

    // 指定順にカメラ演出を再生する。空配列・未登録ID・開始失敗時はfalse。再生中はPlayer操作を制限する。
    bool PlayCinematicChain(const std::vector<std::string>& cinematicSequenceIds);

    // カメラ演出群の終了後、復帰フェード中点でゲーム演出を再生する。未登録ID・開始失敗時はfalse。
    bool PlayCinematicChainThenSequence(const std::vector<std::string>& cinematicSequenceIds,
                                        const std::string& gameplaySequenceId);

    // 再生と操作制限を解除する。shouldRestorePreviewStateがtrueなら退避したActor位置・表示も戻す。
    // 退避対象のActorを破棄する前に呼ぶこと。
    void Stop(bool shouldRestorePreviewState = true);

    bool IsPlaying() const { return mIsPlaying || mIsCinematicChainPlaying; }
    bool IsPreviewing() const { return mIsPreview; }
    bool IsCinematicChainPlaying() const { return mIsCinematicChainPlaying; }
    bool HasPreviewSnapshot() const { return !mPreviewSnapshots.empty(); }
    bool LocksPlayerControl() const { return mLocksPlayerControl; }

    float GetElapsedTime() const { return mElapsedTime; }
    float GetDuration() const;
    const std::string& GetActiveSequenceId() const { return mActiveSequenceId; }
    const std::string& GetLastError() const { return mLastError; }

    SequenceLibrary& GetLibrary() { return mLibrary; }
    const SequenceLibrary& GetLibrary() const { return mLibrary; }

    bool Save() const { return mLibrary.Save(); }

    // 再生を停止し、プレビューを復元してから定義を再読込する。
    bool Reload();

    // 種別と0始まりのインデックスから借用Actorを探す。参照が不正・該当なしならnullptr。
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
    bool StartCinematicChain(const std::vector<std::string>& cinematicSequenceIds,
                             const std::string& gameplaySequenceId);
    void UpdateCinematicChain();
    void RequestCinematicReturnFade();
    void FinishCinematicChainAtFadeMidpoint(std::size_t cinematicChainGeneration);
    void ClearCinematicChain();

    static float ApplyEasing(float progress, SequenceEasing easing);

    Game* mGame = nullptr;
    SequenceLibrary mLibrary;

    const GameplaySequence* mActiveSequence = nullptr;
    std::string mActiveSequenceId;
    std::string mLastError;

    float mElapsedTime = 0.0f;
    bool mIsPlaying = false;
    bool mIsPreview = false;
    bool mLocksPlayerControl = false;

    std::vector<std::string> mCinematicSequenceChain;
    std::size_t mCinematicSequenceIndex = 0;
    std::string mGameplaySequenceAfterCinematics;
    bool mIsCinematicChainPlaying = false;
    bool mIsCinematicReturnFadeRequested = false;
    std::size_t mCinematicChainGeneration = 0;

    std::unordered_map<Actor*, ActorSnapshot> mPreviewSnapshots;
};
