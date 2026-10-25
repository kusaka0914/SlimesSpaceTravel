#pragma once

#include <GL/glew.h>

// CPUの密なピクセル列を2Dテクスチャへ転送する間、現在のbindingとunpack設定を退避する。
// 同じGLコンテキストで使う。所有権は移さず、転送中に破棄された旧bindingは0へ戻す。
class TextureUploadState {
public:
    TextureUploadState();
    ~TextureUploadState();
    TextureUploadState(const TextureUploadState&) = delete;
    TextureUploadState& operator=(const TextureUploadState&) = delete;

private:
    GLint mTexture = 0;
    GLint mUnpackBuffer = 0;
    GLint mAlignment = 0;
    GLint mRowLength = 0;
    GLint mSkipRows = 0;
    GLint mSkipPixels = 0;
};
