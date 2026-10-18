#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <yaml-cpp/yaml.h>

// 惑星の配置・見た目・重力・出現条件をYAMLへ反映する。ファイル保存と編集記録の解除は担当しない。
class StagePlanetYamlWriter {
public:
    explicit StagePlanetYamlWriter(DebugEditorContext& context);

    // 実行中の見た目・重力・出現条件をconfigへ反映する。
    bool WriteSettings(YAML::Node& config) const;

    // 編集記録のある惑星Transformだけをconfigへ反映する。編集記録は解除しない。
    bool WriteEditorAuthoredTransforms(YAML::Node& config) const;

private:
    bool Write(YAML::Node& config, bool shouldSaveEditorTransform) const;

    DebugEditorContext& mContext;
};
