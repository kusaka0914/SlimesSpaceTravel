#pragma once

enum class TextureColorSpace {
    Linear,
    SRGB,
};

class TextureLoader {
public:
    unsigned int LoadTexture(
        const char* path,
        TextureColorSpace colorSpace) const;
};
