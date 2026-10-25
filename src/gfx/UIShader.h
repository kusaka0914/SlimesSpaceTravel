#pragma once

#include "gfx/Shader.h"

// UIの色・テクスチャ・色空間変換を扱うGLSLプログラムとuniform位置を保持する。
class UIShader : public Shader {
public:
    UIShader();
    ~UIShader();

    int GetLocConvertSrgbToLinear() const { return mLocConvertSrgbToLinear; }

private:
    void Initialize();

    int mLocConvertSrgbToLinear = -1;
};
