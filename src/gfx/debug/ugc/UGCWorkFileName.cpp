#include "gfx/debug/ugc/UGCWorkFileName.h"

#include <cstring>
#include <filesystem>

namespace {

std::filesystem::path CreateUtf8Path(const std::string& text)
{
    const std::u8string utf8Text(text.begin(), text.end());
    return std::filesystem::path(utf8Text);
}

std::string ToUtf8String(const std::filesystem::path& path)
{
    const std::u8string utf8Text = path.u8string();
    return std::string(utf8Text.begin(), utf8Text.end());
}

}

namespace UGCWorkFileName {

std::string CreateSafeFileName(const std::string& displayName)
{
    std::string safeName = displayName;
    constexpr const char* invalidCharacters = "<>:\"/\\|?*";
    for (char& character : safeName) {
        const unsigned char unsignedCharacter =
            static_cast<unsigned char>(character);
        if (unsignedCharacter < 32 ||
            std::strchr(invalidCharacters, character)) {
            character = '_';
        }
    }
    while (!safeName.empty() &&
           (safeName.back() == ' ' || safeName.back() == '.')) {
        safeName.pop_back();
    }
    if (safeName.empty()) {
        safeName = "untitled";
    }
    return safeName + ".yaml";
}

std::string ResolveDisplayName(const std::string& fileName)
{
    return ToUtf8String(CreateUtf8Path(fileName).stem());
}

}
