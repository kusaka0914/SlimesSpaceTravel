#pragma once

#include "effect/particle/ParticleTypes.h"

#include <string>
#include <unordered_map>

// エフェクトIDをキーとする粒子定義と、YAMLのparticleEffectsマップを相互変換する。
class ParticleConfigLoader {
public:
    // 読み込み成功時はoutDefinitionsを置き換える。有効なエミッターがない定義は除外するため、空になる場合もある。
    // particleEffectsマップの欠落・形式不正やYAML例外ではfalseを返し、outDefinitionsを変更しない。
    static bool Load(const std::string& filePath,
                     std::unordered_map<std::string, ParticleEffectDefinition>& outDefinitions);

    // 親ディレクトリを作成し、ID順にYAMLを上書き保存する。失敗時はfalseを返す。
    // 書き込み途中の失敗では、既存ファイルの内容は復元しない。
    static bool Save(const std::string& filePath,
                     const std::unordered_map<std::string, ParticleEffectDefinition>& definitions);
};
