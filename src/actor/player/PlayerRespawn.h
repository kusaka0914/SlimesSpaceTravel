#pragma once

#include <glm/glm.hpp>

class Player;
class PlayerCombat;
class PlayerStateMachine;
class PlayerStatus;

// 落下・接地面の未検出による復帰要求と、再開する惑星・位置・向きの保存を担当する。
// 位置・方向はワールド空間。deltaTimeは秒。
class PlayerRespawn {
public:
    // 生存中のみ落下ダメージと共有資源の同期を行い、生き残った場合はSceneSystemへ復帰を要求する。
    void ApplyFallDamageAndRespawn(Player& player, PlayerStatus& status, float damage);

    // 保存位置へ配置する。有効な再開惑星があれば所属も戻す。HP・速度・行動状態のリセットは含まない。
    void Respawn(Player& player);
    bool IsFallIntoPlanetInside(const Player& player) const;

    // prevPosから現在位置までの移動区間で落下判定を行い、接触したFallRespawnPointのダメージを適用する。
    void CheckFallRespawn(Player& player, PlayerStateMachine& stateMachine, PlayerCombat& combat, PlayerStatus& status,
                          const glm::vec3& prevPos);

    // 接地面の未検出が5秒続いたら復帰を要求する。trueは今回の要求受理を表し、要求済みの以後の呼出しではfalse。
    // deltaTimeは秒。レイの検出通知を消費し、復帰完了時にはOnRespawnCompletedで状態を戻す。
    bool UpdateMissingGroundSurfaceRespawn(Player& player, const PlayerStatus& status, float deltaTime);
    void OnGroundSurfaceDetected() { mGroundSurfaceDetectedThisFrame = true; }
    void OnRespawnCompleted();

    void SetRestartPlanetIndex(int restartPlanetIndex) { mRestartPlanetIndex = restartPlanetIndex; }
    void SetRestartPos(const glm::vec3& restartPos) { mRestartPos = restartPos; }
    void CaptureRestartFacingDirection(const Player& player);

    // 保存方向を現在の接平面へ投影し、Playerの向きとカメラ基準方向を戻す。方向を決められなければ何もしない。
    void RestoreRestartFacingDirection(Player& player) const;

    int GetRestartPlanetIndex() const { return mRestartPlanetIndex; }
    const glm::vec3& GetRestartPos() const { return mRestartPos; }

private:
    static constexpr float missingGroundSurfaceRespawnDelaySeconds = 5.0f;

    int mRestartPlanetIndex = 0;
    glm::vec3 mRestartPos = glm::vec3(0.0f);
    glm::vec3 mRestartFacingDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    bool mHasRestartFacingDirection = false;
    float mMissingGroundSurfaceDurationSeconds = 0.0f;
    bool mGroundSurfaceDetectedThisFrame = false;
    bool mRespawnFadeRequested = false;
};
