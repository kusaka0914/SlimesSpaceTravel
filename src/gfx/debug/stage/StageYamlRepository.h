#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <cstddef>
#include <iostream>
#include <string>
#include <yaml-cpp/yaml.h>

// 現在のステージYAMLの読み書きと、シーケンス要素の編集を提供する。Actorの生成・再読み込みは行わない。
class StageYamlRepository {
public:
    static std::string GetCurrentStageYamlPath(DebugEditorContext& context);

    // パスが取得できない場合やYAMLの読み込み・解析失敗時は false。
    static bool LoadCurrentStage(DebugEditorContext& context, YAML::Node& outConfig);

    // preserveRuntimePlanetCenters が true なら、既存の惑星ノードの中心を実行中の位置で更新して保存する。
    // YAML::Nodeは共有参照のため、呼び出し元のconfigにも中心の更新が反映される。
    static bool SaveCurrentStage(DebugEditorContext& context, YAML::Node config,
                                 bool preserveRuntimePlanetCenters = true);

    static bool ReadCurrentStageText(DebugEditorContext& context, std::string& outYamlText);

    // YAMLを検証して一時ファイルへ書き出し、元ファイルと置き換える。失敗時は false。
    // 置換に失敗しても元ファイルを削除しない。
    static bool WriteCurrentStageTextAtomically(DebugEditorContext& context, const std::string& yamlText);

    // Map形式のステージYAMLを読む。読込・検証失敗時はoutConfigを変更しない。
    static bool LoadYamlFile(const std::string& filePath, YAML::Node& outConfig);

    // 旧ファイルを保持して置換する。書込み・置換失敗時はfalse。
    static bool SaveYamlFile(const std::string& filePath, const YAML::Node& config);

    // メモリ上の要素を削除し、後続の添字を詰める。不正なシーケンス・添字には false。
    static bool RemoveSequenceElement(YAML::Node& config, const std::string& sequenceName, int index);

    // メモリ上の指定要素のキーを更新する。不正なシーケンス・添字には false。保存は行わない。
    template <typename T>
    static bool SetSequenceValue(YAML::Node& config, const std::string& sequenceName, std::size_t index,
                                 const std::string& key, const T& value)
    {
        if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
            std::cerr << "Invalid yaml sequence: " << sequenceName << std::endl;
            return false;
        }

        if (index >= config[sequenceName].size()) {
            std::cerr << "Index out of range: " << index << std::endl;
            return false;
        }

        config[sequenceName][index][key] = value;
        return true;
    }
};
