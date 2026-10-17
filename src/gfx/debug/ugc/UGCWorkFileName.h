#pragma once

#include <string>

namespace UGCWorkFileName {

// 表示名の禁止文字を「_」へ置換し、末尾の空白・ピリオドを除いて.yamlを付ける。空ならuntitledを使う。
std::string CreateSafeFileName(const std::string& displayName);

// UTF-8のファイル名から拡張子を除いた表示名を返す。
std::string ResolveDisplayName(const std::string& fileName);

} // namespace UGCWorkFileName
