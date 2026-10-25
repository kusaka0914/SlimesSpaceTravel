#pragma once

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct PlatformTypeDefinition {
    std::string id;
    std::string displayName;
    std::string sequenceName;
    YAML::Node defaults;
};

// 足場の種類と初期値を読み込み、エディターで共通利用する。定義の読み込み失敗時は組み込み値を使う。
class PlatformTypeRegistry {
public:
    // 初回呼び出し時に読み込んだ定義を返す。返却参照は以後の呼び出しでも有効。
    static const std::vector<PlatformTypeDefinition>& GetDefinitions();

    // 未登録のシーケンス名には nullptr を返す。返却ポインタはレジストリが保持する定義を指す。
    static const PlatformTypeDefinition* FindBySequenceName(const std::string& sequenceName);

    // 既存値を上書きせず、不足する設定だけを再帰的に補う。
    static void ApplyDefaults(YAML::Node& platformNode, const PlatformTypeDefinition& definition);

private:
    static std::vector<PlatformTypeDefinition> LoadDefinitions();
    static void MergeMissingValues(YAML::Node& target, const YAML::Node& defaults);
};
