#pragma once

#include "Shader.h"

class UIShader : public Shader {
public:
    UIShader();
    ~UIShader();

    void Initialize();
    int GetLocConvertSrgbToLinear() const
    {
        return mLocConvertSrgbToLinear;
    }

private:
    int mLocConvertSrgbToLinear = -1;
};
