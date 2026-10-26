#pragma once

#include "gfx/Shader.h"

// UIの色・テクスチャ・色空間変換を扱うGLSLプログラムとuniform位置を保持する。
class UIShader : public Shader {
public:
    UIShader();
    ~UIShader();

    int GetLocDamageVignette() const { return mLocDamageVignette; }
    int GetLocFlashStrength() const { return mLocFlashStrength; }
    int GetLocFlashColor() const { return mLocFlashColor; }
    int GetLocPanelSize() const { return mLocPanelSize; }
    int GetLocBorderWidth() const { return mLocBorderWidth; }
    int GetLocBorderColor() const { return mLocBorderColor; }
    int GetLocCornerRadius() const { return mLocCornerRadius; }
    int GetLocGrayscale() const { return mLocGrayscale; }
    int GetLocConvertSrgbToLinear() const { return mLocConvertSrgbToLinear; }

private:
    void Initialize();

    int mLocConvertSrgbToLinear = -1;
    int mLocGrayscale = -1;
    int mLocPanelSize = -1;
    int mLocCornerRadius = -1;
    int mLocBorderWidth = -1;
    int mLocBorderColor = -1;
    int mLocDamageVignette = -1;
    int mLocFlashStrength = -1;
    int mLocFlashColor = -1;
};
