#pragma once

#include <yaml-cpp/yaml.h>

namespace YamlRead {

// キーが未定義の場合だけfallbackを返す。明示的なnullもas<T>()で変換し、変換・参照エラーは呼出元へ伝える。
// 範囲制限や既定値の選択は設定ごとの呼出元で行う。
template <typename T> T ReadOr(const YAML::Node& node, const char* key, const T& fallback = T{})
{
    const YAML::Node value = node[key];
    return value ? value.as<T>() : fallback;
}

} // namespace YamlRead
