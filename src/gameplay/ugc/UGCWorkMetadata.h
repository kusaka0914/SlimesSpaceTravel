#pragma once

#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

namespace UGCWorkMetadata {

// 表示名とファイル名を設定する。既存の完成チェック状態は維持し、フラグが未定義の場合だけ未検証にする。
void PrepareForSave(YAML::Node& stageYaml, const std::string& displayName, const std::string& fileName);

// 名前が未定義・形式不正の場合はnulloptを返す。
std::optional<std::string> FindDisplayName(const YAML::Node& stageYaml);
std::optional<std::string> FindFileName(const YAML::Node& stageYaml);
void InvalidateClearVerification(YAML::Node& stageYaml);
void MarkClearVerified(YAML::Node& stageYaml);
bool IsClearVerified(const YAML::Node& stageYaml);
bool HasGoal(const YAML::Node& stageYaml);

} // namespace UGCWorkMetadata
