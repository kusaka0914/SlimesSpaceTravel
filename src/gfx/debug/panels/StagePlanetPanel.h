#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <functional>
#include <string>

class Planet;

// 惑星の形状・外観・配置条件を編集し、所属Actorの追従と編集済みTransformの保存を連携する。
class StagePlanetPanel : public DebugPanel {
public:
    // 保存処理は再試行まで保持するため、通知先の寿命を保証すること。
    StagePlanetPanel(DebugEditorContext& context, std::function<bool()> saveEditorTransforms);

    void Draw() override;

    // 指定惑星に絞って編集UIを描画する。nullptrなら何もしない。惑星は所有しない。
    void DrawSelectedPlanet(Planet* selectedPlanet);

    // 依存Actorと惑星Transformをまとめて保存する処理を呼ぶ。
    // 失敗時はfalseで編集記録を保持する。編集がなければtrue。
    bool SaveEditorAuthoredTransforms();

private:
    void DrawPlanetProperties(Planet* planet, std::size_t planetIndex);
    void DrawTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawBackTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex);

    Planet* mFocusedPlanet = nullptr;
    std::array<char, 128> mTextureAssetFilter = {};
    std::string mTextureAssetStatus;
    bool mHasPendingTransformEdit = false;
    std::string mTransformSaveError;
    std::function<bool()> mSaveEditorTransforms;
};
