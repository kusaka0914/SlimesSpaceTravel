#pragma once

#include "gfx/debug/DebugEditorContext.h"

// 惑星の編集済み配置、または見た目・重力・ロケット出現条件をステージYAMLへ保存する。
class StagePlanetYamlWriter {
public:
    explicit StagePlanetYamlWriter(DebugEditorContext& context);

    // true なら編集記録のある配置、false なら見た目・重力・出現条件を保存する。
    // どちらも実行中の惑星中心を保存する。編集記録は消さず、読み書き失敗時は false。
    bool Save(bool shouldSaveEditorTransform) const;

private:
    DebugEditorContext& mContext;
};
