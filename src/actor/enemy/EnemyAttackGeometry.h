#pragma once

#include <glm/glm.hpp>

class Enemy;
class Planet;

// 攻撃判定・予告の基準位置と方向（ワールド空間）。方向には単位ベクトルを使う。
struct EnemyAttackFrame {
    glm::vec3 origin{0.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, 1.0f};
    glm::vec3 left{1.0f, 0.0f, 0.0f};
};

// 基準位置からの前方開始距離、そこからの長さ、左右の半幅（ワールド単位）。
struct EnemyMeleeAttackPreviewArea {
    float forwardStartOffset = 0.0f;
    float forwardLength = 0.0f;
    float halfWidth = 0.0f;
};

// Enemyの現在の位置・向きから判定基準を作る。上方向や前方向を決められない場合は基準軸で代用する。
EnemyAttackFrame ResolveEnemyAttackFrame(const Enemy& enemy);

// モデル前端までのワールド距離。境界情報がなければEnemyの半径で代用する。
float CalculateEnemyAttackFrontOffset(
    const Enemy& enemy,
    const EnemyAttackFrame& attackFrame);

// Enemyのモデル幅とPlayerの衝突半径を含めた、近接攻撃の予告・命中判定用の範囲を求める。
EnemyMeleeAttackPreviewArea CalculateEnemyMeleeAttackPreviewArea(
    const Enemy& enemy,
    const EnemyAttackFrame& attackFrame);

// attackFrameの平面へ投影して前方の長方形を判定する。長さは正、半幅は0以上でなければfalse。
bool IsPositionInsideMeleeAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth);

// attackFrameの平面へ投影して扇形を判定する。angleRadiansは扇形全体の角度。範囲・角度が非正ならfalse。
bool IsPositionInsideFanAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians);

// attackFrameの平面へ投影して円内を判定する。rangeが非正ならfalse。
bool IsPositionInsideRadialAttack(
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range);

// 球形惑星の中心から、攻撃予告の基準平面へpositionを投影して判定する。
// 球以外・惑星中心付近・基準平面の反対側など、投影できない位置ではfalse。
bool IsPositionInsideSphereSurfaceMeleeAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float forwardLength,
    float halfWidth);
bool IsPositionInsideSphereSurfaceFanAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range,
    float angleRadians);
bool IsPositionInsideSphereSurfaceRadialAttack(
    const Planet& planet,
    const EnemyAttackFrame& attackFrame,
    const glm::vec3& position,
    float range);
