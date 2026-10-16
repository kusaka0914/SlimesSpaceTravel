#pragma once

class Boat;
class Player;
class PlayerInput;
class PlayerMovement;
class PlayerRespawn;

// 乗船・発進待ち・移動中の追従・降船を管理し、到着先を再開位置へ記録する。
class PlayerBoatRide {
public:
    // deltaTimeは秒。降船入力を先に扱い、乗船・追従を更新する。
    void Update(
        Player& player,
        PlayerInput& input,
        PlayerMovement& movement,
        PlayerRespawn& respawn,
        float deltaTime);

    // boatは非null。船の位置へ合わせ、Playerを非アクティブにする。
    void FollowMovingBoat(Player& player, Boat* boat) const;
    bool IsWaitingForBoat(const Player& player) const;

    // 発進待ちなら船外へ移動して操作・着地判定を再開し、0.5秒間の再乗船を禁止する。失敗時はfalse。
    bool CancelWaitingBoatRide(Player& player);

    // boatは非null。ワールド位置間の距離で乗船範囲を判定する。
    bool IsTouchingBoat(const Player& player, Boat* boat) const;

    // 乗船後はPlayerを非アクティブにし、分裂・2人プレイでは全員の搭乗を待って発進する。
    // boatがnullptr、またはPlayerが非アクティブなら何もしない。
    void StartRidingBoat(Player& player, Boat* boat) const;

    // 到着先の惑星・位置と再開位置を更新し、Playerを再有効化する。船または到着惑星なしでは何もしない。
    void OnBoatArrived(Player& player, PlayerMovement& movement, PlayerRespawn& respawn, Boat* boat) const;

private:
    Boat* FindWaitingBoat(const Player& player) const;

    float mReboardCooldownSeconds = 0.0f;
};
