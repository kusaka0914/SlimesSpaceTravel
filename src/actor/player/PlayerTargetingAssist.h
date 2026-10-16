#pragma once

class Enemy;
class Player;
class PlayerMovement;

// アシスト操作の攻撃対象を選び、対象へ向けるための接平面方向を計算する。
// 検索結果は敵への非所有参照。候補なしではnullptrを返す。
class PlayerTargetingAssist {
public:
    // 同じ惑星面の攻撃範囲内で正面の敵を優先し、なければ全方向から選ぶ。角度はラジアン。
    // requireLaunchedTargetがtrueなら、打ち上げ中の敵に限定する。
    static Enemy* FindAttackTarget(
        const Player& player,
        float attackRange,
        float attackAngle,
        bool requireLaunchedTarget);

    // ガードを失った打ち上げ中の敵から、正面を優先して選ぶ。maxDistanceはワールド距離、角度はラジアン。
    static Enemy* FindAssistStrongTarget(
        const Player& player,
        float maxDistance,
        float attackAngle);

    // 現在の惑星全体からモデル境界まで最も近い打ち上げ中の敵を選ぶ。表裏の一致は要求しない。
    static Enemy* FindNearestLaunchedTargetOnCurrentPlanet(
        const Player& player);

    // 復帰までの残り時間が指定秒数以内の敵を選ぶ。残り時間が負なら下降中の敵を候補にする。
    static Enemy* FindNearestLaunchedTargetNearRecoveryOnCurrentPlanet(
        const Player& player,
        float maximumLaunchedTimerSeconds);

    // 接平面上で対象へ向ける。方向を決められなければfalseを返し、向きを変えない。
    static bool FaceTarget(Player& player, PlayerMovement& movement, const Enemy& target);
};
