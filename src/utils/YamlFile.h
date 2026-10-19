#pragma once

#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace YamlFile {

// 通常ファイルを解析する。パス不正・I/O失敗はYAML::BadFile、構文エラーはYAML::Exceptionで通知する。
// 返されたNodeの採用や既存データの保持は呼出元で決める。
YAML::Node Load(const std::filesystem::path& path);

} // namespace YamlFile
