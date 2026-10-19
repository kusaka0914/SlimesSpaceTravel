#include "utils/TextFile.h"

#include <array>
#include <fstream>
#include <utility>

namespace TextFile {

bool Read(const std::filesystem::path& path, std::string& outText, std::string& outError)
{
    outError.clear();
    if (path.empty() ||
        path.native().find(std::filesystem::path::value_type{}) != std::filesystem::path::string_type::npos) {
        outError = "The file path is empty or contains an embedded null.";
        return false;
    }
    std::error_code statusError;
    if (!std::filesystem::is_regular_file(path, statusError)) {
        outError = statusError ? statusError.message() : "Expected a regular file.";
        return false;
    }
    try {
        // 保存前のスナップショットにも使うため、Windowsでも改行を変換しない。
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            outError = "Failed to open the file.";
            return false;
        }
        std::string text;
        std::array<char, 4096> buffer{};
        while (input.read(buffer.data(), buffer.size())) {
            text.append(buffer.data(), buffer.size());
        }
        text.append(buffer.data(), static_cast<std::size_t>(input.gcount()));
        if (!input.eof() || input.bad()) {
            outError = "Failed to read the complete file.";
            return false;
        }
        outText = std::move(text);
        return true;
    } catch (const std::ios_base::failure& error) {
        outError = error.what();
        return false;
    }
}

} // namespace TextFile
