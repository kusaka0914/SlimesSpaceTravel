#pragma once

#include "gfx/Shader.h"

// UIの色・テクスチャ・色空間変換を扱うGLSLプログラムとuniform位置を保持する。
class UIShader : public Shader {
public:
    UIShader();
    ~UIShader();

    // プログラムとuniform位置を設定する。コンストラクタから呼ばれる初期化処理。
    void Initialize();
    int GetLocConvertSrgbToLinear() const { return mLocConvertSrgbToLinear; }

private:
    int mLocConvertSrgbToLinear = -1;
};
