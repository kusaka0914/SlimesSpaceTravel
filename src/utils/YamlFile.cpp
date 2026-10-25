#include "utils/YamlFile.h"

#include "utils/TextFile.h"

namespace YamlFile {

YAML::Node Load(const std::filesystem::path& path)
{
    std::string text;
    std::string error;
    if (!TextFile::Read(path, text, error)) {
        // OSや標準ライブラリで異なるI/O例外を、既存のYAML読込契約へ揃える。
        throw YAML::BadFile(error);
    }
    return YAML::Load(text);
}

} // namespace YamlFile
