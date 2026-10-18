#pragma once

#include "editor/core/DebugEditorContext.h"

#include <glm/glm.hpp>

class UGCEditorTutorial;

// UGC編集カメラの方向・投影方式・ズームを制御する。contextとチュートリアルは所有しない。
class UGCEditorViewController {
public:
    UGCEditorViewController(DebugEditorContext& context, UGCEditorTutorial& editorTutorial);

    // 正の倍率で拡大率を調整する。正投影では表示範囲、透視投影では視点距離を変更する。
    void AdjustZoom(float distanceMultiplier);

    // 上下視点を切り替え、正投影とフリーカメラを有効にする。Gameまたはカメラ未初期化時は何もしない。
    void ToggleVerticalView();

    // ゼロでないviewDirectionを対象から視点への方向として使う。軸に沿う視点は正投影、それ以外は透視投影。
    // Game・カメラ・ステージが揃う場合に視点を再設定し、フリーカメラを有効にする。
    void SetFixedView(const glm::vec3& viewDirection);
    const glm::vec3& GetViewDirection() const;

private:
    void AdjustViewDistance(float distanceMultiplier);

    DebugEditorContext& mContext;
    UGCEditorTutorial& mEditorTutorial;
    glm::vec3 mViewDirection{0.0f, 1.0f, 0.0f};
};
