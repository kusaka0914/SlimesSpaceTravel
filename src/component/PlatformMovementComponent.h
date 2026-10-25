#pragma once

#include "component/Component.h"

#include <glm/glm.hpp>

class Platform;

// 始点と終点の往復、乗員による移動開始、エディターでの移動プレビューを管理する。
// LocalPosとMoveOffsetは惑星中心からの平行移動座標・変位で、惑星の回転やスケールは適用しない。時間は秒。
class PlatformMovementComponent : public Component {
public:
    explicit PlatformMovementComponent(Platform* owner, int updateOrder = 50);

    void Update(float deltaTime) override;

    void SetMoveOffset(const glm::vec3& moveOffset) { mMoveOffset = moveOffset; }
    const glm::vec3& GetMoveOffset() const { return mMoveOffset; }

    // 自動往復では往復分、乗員起動では片道分の移動時間。端点の待機時間は含まず、0.1秒以上に制限する。
    void SetMoveDuration(float moveDuration);
    float GetMoveDuration() const { return mMoveDuration; }

    void SetBaseLocalPos(const glm::vec3& baseLocalPos) { mBaseLocalPos = baseLocalPos; }

    // 惑星中心基準の変位で経路の始点・終点をまとめて移す。床の現在位置は変更しない。
    void TranslatePath(const glm::vec3& localDelta) { mBaseLocalPos += localDelta; }
    const glm::vec3& GetBaseLocalPos() const { return mBaseLocalPos; }

    void SetMoveOnPlayer(bool moveOnPlayer) { mMoveOnPlayer = moveOnPlayer; }
    bool GetMoveOnPlayer() const { return mMoveOnPlayer; }

    // 乗員が降りて戻り始めるまでの待ち時間（秒）。負値は0に制限する。
    void SetReturnDelay(float returnDelay);
    float GetReturnDelay() const { return mReturnDelay; }

    // 端点での待機時間（秒）。負値は0に制限し、乗員起動では降りた後の戻り待ち時間とは別に扱う。
    void SetEndpointWaitDurationSeconds(float endpointWaitDurationSeconds);
    float GetEndpointWaitDurationSeconds() const { return mEndpointWaitDurationSeconds; }

    glm::vec3 GetDestinationLocalPos() const { return mBaseLocalPos + mMoveOffset; }
    void SetDestinationLocalPos(const glm::vec3& destinationLocalPos)
    {
        mMoveOffset = destinationLocalPos - mBaseLocalPos;
    }

    // 1なら終点、それ以外は始点を選び、再生を止めて床をその位置へ配置する。フレーム変位は0にする。
    void SetEditorPreviewPoint(int point);
    int GetEditorPreviewPoint() const { return mEditorPreviewPoint; }

    // 選択中の端点を変更する。始点を変更する場合も、変更前の終点は維持する。
    void SetEditorPreviewLocalPos(const glm::vec3& localPos);

    // 始点へ戻して片道プレビューを開始する。終点到達で再生を止め、終点を選択状態にする。
    void StartEditorMovementPreview();

    // 再生フラグと再生時間だけを戻す。床の現在位置と端点の選択状態は変えない。
    void StopEditorMovementPreview();

    // 編集画面かつフリーカメラ中だけ、再生中のプレビューを進める。deltaTimeは秒。
    void UpdateEditorMovementPreview(float deltaTime);
    bool IsEditorMovementPreviewPlaying() const { return mIsEditorMovementPreviewPlaying; }

    // このComponentが直近に適用したワールド変位。他Componentによる移動は含まない。
    const glm::vec3& GetFrameDelta() const { return mFrameDelta; }

private:
    bool UpdateEditorPreview(float deltaTime);
    void AdvanceEditorMovementPreview(float deltaTime);
    void ResetRuntimeMovementAfterEditorPreview();
    void UpdateAutomaticMovement(float deltaTime);
    void UpdatePlayerActivatedMovement(float deltaTime);
    bool HasPlayerOnPlatform() const;
    void SetLocalPosition(const glm::vec3& localPos, bool calculateFrameDelta = true);

    Platform* mPlatform = nullptr;

    glm::vec3 mBaseLocalPos{0.0f};
    glm::vec3 mMoveOffset{4.0f, 0.0f, 0.0f};
    glm::vec3 mFrameDelta{0.0f};

    float mMoveDuration = 3.0f;
    float mMoveTimer = 0.0f;
    float mReturnDelay = 1.0f;
    float mReturnDelayTimer = 0.0f;
    float mEndpointWaitDurationSeconds = 0.0f;
    float mEndpointWaitElapsedSeconds = 0.0f;
    float mStartEndpointWaitRemainingSeconds = 0.0f;
    float mTravelProgress = 0.0f;

    bool mMoveOnPlayer = false;
    bool mHasBeenActivated = false;
    bool mWasEditorPreviewing = false;
    bool mIsEditorMovementPreviewPlaying = false;
    float mEditorMovementPreviewElapsedSeconds = 0.0f;
    int mEditorPreviewPoint = 0;
};
