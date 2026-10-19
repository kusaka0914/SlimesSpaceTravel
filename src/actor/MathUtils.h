#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Actor;

// Actorの姿勢・地表基準の編集回転・ビルボード行列を計算する。渡されたActorは所有しない。
class MathUtils {
public:
    // 正規化された up を上方向とする基準姿勢に対し、direction のヨー角をラジアンで返す。
    // direction は up と平行でない方向を渡す。
    float GetYawFromDirection(const glm::vec3& up, const glm::vec3& direction) const;

    // Actorの姿勢へモデル軸補正と描画用回転オフセットを適用する。nullptr には単位行列を返す。
    glm::mat4 CreateOrient(Actor* actor) const;

    // 地表基準のオイラー角（各軸ラジアン）をワールド姿勢へ変換する。
    // actor が nullptr または所属惑星がない場合は、ワールドY軸を上方向とする基準を使う。
    glm::quat CalculateActorOrientationFromEditorRotation(Actor* actor, const glm::vec3& rotationRadians) const;

    // ワールド姿勢を同じ地表基準のオイラー角（各軸ラジアン）へ戻す。長さがほぼ0の姿勢にはゼロを返す。
    glm::vec3 CalculateActorEditorRotationFromOrientation(Actor* actor, const glm::quat& orientation) const;

    // 地表基準の編集回転から、ワールド座標の正規化された上方向を返す。
    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRadians) const;

    // 保存された編集回転をActorの姿勢へ反映する。nullptr なら何もしない。
    void ApplyActorEditorRotation(Actor* actor) const;

    // Actorの位置から上・右方向へ移動した描画位置に、カメラを向く行列を作る。距離・大きさはワールド単位。
    // actorは非null、位置・寸法・余白は有限値。視線の退化時はカメラの軸、無効な上方向はワールドY軸で補う。
    glm::mat4 CreateBillBoard(const glm::mat4& viewMatrix, const Actor* actor, float upMargin, float rightMargin,
                              float width, float height) const;

    // 有限の座標・寸法を指定する。ほぼ0または非有限のupDirectionにはワールドY軸を使う。
    // 中心とカメラが一致する場合はカメラのZ軸、無効なviewMatrixにはワールドZ軸を視線の代わりに使う。
    glm::mat4 CreateBillBoard(const glm::mat4& viewMatrix, const glm::vec3& centerPosition,
                              const glm::vec3& upDirection, float width, float height) const;
};
