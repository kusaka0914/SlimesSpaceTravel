#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StagePlanetYamlWriter.h"

#include <array>
#include <functional>
#include <string>

class Planet;

// 惑星の形状・外観・配置条件を編集し、所属Actorの追従と編集済みTransformの保存を連携する。
class StagePlanetPanel : public DebugPanel {
public:
    explicit StagePlanetPanel(DebugEditorContext& context);

    void Draw() override;

    // 指定惑星に絞って編集UIを描画する。nullptrなら何もしない。惑星は所有しない。
    void DrawSelectedPlanet(Planet* selectedPlanet);

    // 現在の惑星設定をYAMLへ保存する。
    void Save();

    // 依存Actorの保存コールバックを先に呼び、編集で記録した惑星Transformを保存する。
    // 惑星の保存に成功した場合だけ、その編集済み記録を解除する。
    void SaveEditorAuthoredTransforms();

    // 惑星Transformの保存前に実行する処理を保持する。キャプチャ先の寿命は呼び出し側で保証する。
    void SetSaveDependentActorTransformsCallback(std::function<void()> callback);

private:
    void DrawPlanetProperties(Planet* planet, std::size_t planetIndex);
    void DrawTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawBackTexturePicker(Planet* planet, std::size_t planetIndex);
    void DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex);

    Planet* mFocusedPlanet = nullptr;
    StagePlanetYamlWriter mYamlWriter;
    std::array<char, 128> mTextureAssetFilter = {};
    std::string mTextureAssetStatus;
    bool mHasPendingTransformEdit = false;
    std::function<void()> mSaveDependentActorTransformsCallback;
};
