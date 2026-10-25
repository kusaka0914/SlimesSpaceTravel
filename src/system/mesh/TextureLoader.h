#pragma once

enum class TextureColorSpace {
    Linear,
    SRGB,
};

// 画像を読み込み、指定した色空間でOpenGLテクスチャを作成する。
class TextureLoader {
public:
    // 有効なOpenGLコンテキスト上で呼ぶ。画像読込失敗時は0。成功時のテクスチャ解放は呼び出し側で行う。
    // 画像を上下反転して読み込み、呼出前の2D binding・unpack設定を維持する。
    unsigned int LoadTexture(const char* path, TextureColorSpace colorSpace) const;
};
