#pragma once

#include "actor/StageObject.h"
#include "system/camera/zones/CameraZoneSettings.h"

class Player;

// 衝突を持たないカメラ切替用の回転ボックス。Editorだけで半透明表示し、通常プレイでは不可視。
class CameraZone final : public StageObject {
public:
    explicit CameraZone(Game* game);
    void UpdateActor(float) override {}
    float GetRenderOpacity() const override;
    // ワールド座標を回転ボックスで判定する。worldMarginは退出時に各面を広げる距離。
    bool Contains(const glm::vec3& point, float worldMargin = 0.0f) const;

    // 現在のカメラ姿勢を基準Playerの地面法線・相対距離へ変換する。Fixedだけはワールド位置を保存する。
    void CaptureCamera(const CameraPose& pose, const Player& referencePlayer, bool returnCamera = false);
    CameraZoneSettings& GetSettings() { return mSettings; }
    const CameraZoneSettings& GetSettings() const { return mSettings; }

private:
    CameraZoneSettings mSettings;
};
