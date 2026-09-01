#include <GL/glew.h>

#include "gfx/debug/panels/StarCollectionDebugPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "actor/Star.h"
#include "gfx/debug/DebugBuildRestartPanel.h"
#include "imgui.h"

StarCollectionDebugPanel::StarCollectionDebugPanel(
    DebugEditorContext& context,
    DebugBuildRestartPanel& buildRestartPanel)
    : mContext(context),
      mBuildRestartPanel(buildRestartPanel),
      mYamlWriter(context)
{
}

void StarCollectionDebugPanel::Draw()
{
    if (!mContext.game) {
        return;
    }

    Star* star = nullptr;
    if (Stage* stage = mContext.game->GetCurrentStage()) {
        for (Planet* planet : stage->GetPlanets()) {
            if (planet && planet->GetStar()) {
                star = planet->GetStar();
                break;
            }
        }
    }
    if (!star) {
        ImGui::TextUnformatted("現在のステージに星がありません。");
        return;
    }

    Star::CollectionAnimationSettings settings =
        star->GetCollectionAnimationSettings();
    ImGui::TextUnformatted("星獲得演出（カメラは現在のまま）");
    ImGui::DragFloat(
        "周回時間",
        &settings.orbitDuration,
        0.01f,
        0.05f,
        10.0f,
        "%.2f 秒");
    ImGui::DragFloat(
        "周回開始半径",
        &settings.orbitStartRadius,
        0.01f,
        0.0f,
        10.0f);
    ImGui::DragFloat(
        "周回中の横回転速度",
        &settings.orbitSpinDegreesPerSecond,
        5.0f,
        -3600.0f,
        3600.0f,
        "%.0f 度/秒");
    ImGui::DragFloat(
        "真上の高さ",
        &settings.finalHeight,
        0.01f,
        0.0f,
        10.0f);
    ImGui::DragFloat(
        "真上で待つ時間",
        &settings.waitAbovePlayerDuration,
        0.01f,
        0.0f,
        5.0f,
        "%.2f 秒");
    ImGui::DragFloat(
        "落下時間",
        &settings.fallDuration,
        0.01f,
        0.05f,
        10.0f,
        "%.2f 秒");
    star->SetCollectionAnimationSettings(settings);

    if (ImGui::Button("プレビュー再生")) {
        star->StartCollectionPreview(mContext.game->GetMainPlayer());
    }
    ImGui::SameLine();
    if (ImGui::Button("stars.yamlへ保存")) {
        const bool saved =
            mYamlWriter.SaveStarCollectionAnimation(*star);
        mBuildRestartPanel.SetStatus(
            saved
                ? "星獲得演出を保存しました"
                : "星獲得演出を保存できませんでした",
            !saved);
    }
}
