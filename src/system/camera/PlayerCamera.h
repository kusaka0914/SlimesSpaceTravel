#pragma once

#include "system/camera/EllipseCameraTransition.h"

#include <cstddef>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <vector>

class CameraCollisionResolver;
class Player;
struct PlayerCameraSettings;
struct CameraPose;

struct PlayerCameraState {
    EllipseCameraTransition ellipseTransition;

    // followPositionは衝突補正前の基準位置、cameraPosは実際に描画する位置。
    glm::vec3 followPosition{0.0f};
    glm::vec3 desiredFollowPosition{0.0f};
    glm::vec3 cameraPos{0.0f};
    glm::vec3 targetPos{0.0f};
    glm::vec3 lookTarget{0.0f};
    glm::vec3 previousPlayerPosition{0.0f};
    glm::vec3 lateralFollowStart{0.0f};
    glm::vec3 previousPlayerUp{0.0f, 1.0f, 0.0f};
    glm::vec3 upVec{0.0f, 1.0f, 0.0f};
    glm::vec3 cameraForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 alignTargetForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 attackTargetForwardVec{0.0f, 0.0f, 1.0f};
    float collisionRecoveryBlend = 1.0f;
    float distance = 0.0f;
    float pitch = 0.0f;
    float targetHeight = 0.0f;
    float manualYaw = 0.0f;
    float attackCameraManualDelay = 0.0f;
    float lateralFollowBlend = 1.0f;
    float clearanceRadius = 0.3f;
    bool ownsEllipseMovementLock = false;
    bool followsLateralMovement = false;
    bool needsNormalFraming = false;
    bool hasFollowPosition = false;
    bool hasPlayerPosition = false;
    bool hasCameraForward = false;
    bool hasAttackTargetForward = false;
    bool manualInputPending = false;
    bool isRecoveringFromCollision = false;
    bool isAligningBehindPlayer = false;
    bool isTransitioningToPlayer = false;
    bool isSettlingJump = false;
    bool orbitRequested = false;
    bool isBackwardFacingFramingActive = false;
    float backwardMovementHoldSeconds = 0.0f;
    float backwardFacingLookAheadDistance = 0.0f;
};

// Sphereは全方向へ位置追従し、その他は前後位置・左右視線を追う。Playerインデックスは0始まり。
class PlayerCamera {
public:
    explicit PlayerCamera(CameraCollisionResolver& collisionResolver);

    // Player順のyawDeltas（ラジアン）と秒単位のdeltaTimeで追従状態を更新する。
    // dedicatedCamerasがtrueのPlayerには、攻撃時補助・楕円惑星の回り込みを重ねない。
    void Update(const std::vector<Player*>& players, const std::vector<float>& yawDeltas,
                const PlayerCameraSettings& settings, float deltaTime, bool allowsMovementCameraAssist,
                const std::vector<bool>& dedicatedCameras = {});
    void Reset();

    // 専用演出への移行時に回り込みを中止し、この処理が取得した移動方向保持だけを解除する。
    void CancelEllipseTransitions(const std::vector<Player*>& players);

    // ゾーンの表示姿勢を引き継ぐ。次の通常ビューは、その向きで通常の距離・ピッチへ戻す復帰先になる。
    void AdoptZonePose(int playerIndex, const CameraPose& pose, const Player& player);

    // 手動操作を優先して回り込み・背面合わせを解除し、攻撃時補助を一時停止する。
    // nullptr・負の添字では何もしない。
    void NotifyManualInput(Player* player, int playerIndex);

    // 指定Playerの姿勢へ追従状態を初期化する。nullptr・負のインデックスでは何もしない。
    void SnapToPlayer(Player* player, int playerIndex);
    void TransitionToPlayer(int fromPlayerIndex, Player* player, int toPlayerIndex);
    void SnapBehindPlayer(Player* player, int playerIndex);
    void AlignBehindPlayer(Player* player, int playerIndex);
    void BlendBehindTarget(Player* player, int playerIndex, const glm::vec3& targetPosition, float blend);

    // 追従状態と衝突補正からビューを生成し、カメラ位置を更新する。playerIndexは0以上、cameraPitchはラジアン。
    glm::mat4 GetView(Player* player, int playerIndex, float cameraDistance, float cameraPitch, float targetHeight,
                      bool isFixed = false, float clearanceRadius = 0.3f);

    // 0始まりのインデックスで描画位置・注視点を返す。範囲外ならゼロベクトル。
    glm::vec3 GetCameraPos(int playerIndex) const;
    glm::vec3 GetLookTarget(int playerIndex) const;

private:
    void ResizeState(std::size_t count);
    void UpdateState(Player* player, int playerIndex, const PlayerCameraSettings& settings, float deltaTime,
                     bool allowsMovementCameraAssist, bool dedicatedCamera);
    void UpdateFollowPosition(Player* player, PlayerCameraState& state, const PlayerCameraSettings& settings,
                              float deltaTime);
    void UpdateCameraForward(Player* player, PlayerCameraState& state, float deltaTime);
    void UpdateEllipseTransition(Player& player, PlayerCameraState& state, const PlayerCameraSettings& settings,
                                 float deltaTime, float movementSpeed, bool allowsAssist, bool dedicatedCamera);
    void UpdateAttackCameraAssist(Player* player, PlayerCameraState& state, const PlayerCameraSettings& settings,
                                  float deltaTime, bool allowsAssist);
    void UpdateBackwardFacingFraming(Player* player, PlayerCameraState& state, const PlayerCameraSettings& settings,
                                     float deltaTime, bool allowsMovementCameraAssist);

    CameraCollisionResolver& mCollisionResolver;
    std::vector<PlayerCameraState> mStates;
};
