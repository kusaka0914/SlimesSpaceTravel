#pragma once

#include "actor/StageObject.h"
#include "system/camera/zones/CameraZoneSettings.h"

// 衝突を持たないカメラ切替用の回転ボックス。Editorだけで半透明表示し、通常プレイでは不可視。
class CameraZone final : public StageObject {
public:
    explicit CameraZone(Game* game);
    void UpdateActor(float) override {}
    float GetRenderOpacity() const override;
    // ワールド座標を回転ボックスで判定する。worldMarginは退出時に各面を広げる距離。
    bool Contains(const glm::vec3& point, float worldMargin = 0.0f) const;

    CameraZoneSettings& GetSettings() { return mSettings; }
    const CameraZoneSettings& GetSettings() const { return mSettings; }

private:
    CameraZoneSettings mSettings;
};
