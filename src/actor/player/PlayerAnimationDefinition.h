#pragma once

#include <cctype>
#include <string_view>

#include <string>
#include <unordered_map>

enum class PlayerAnimationPlaybackMode {
    BaseLoop,
    OneShot,
};

// 論理IDに対応するモデル内のクリップ名と再生方式。IDとの対応はPlayerAnimationDefinitionsで保持する。
struct PlayerAnimationDefinition {
    std::string clipName;
    PlayerAnimationPlaybackMode playbackMode = PlayerAnimationPlaybackMode::OneShot;
};

using PlayerAnimationDefinitions = std::unordered_map<std::string, PlayerAnimationDefinition>;

// 論理ID・再生方式の照合用に、空白とハイフン/アンダースコアを除きCロケールで小文字化する。
inline std::string NormalizePlayerAnimationId(std::string_view animationId)
{
    std::string normalizedId;
    normalizedId.reserve(animationId.size());

    for (const unsigned char character : animationId) {
        if (std::isspace(character) || character == '-' || character == '_') {
            continue;
        }
        normalizedId.push_back(static_cast<char>(std::tolower(character)));
    }

    return normalizedId;
}
