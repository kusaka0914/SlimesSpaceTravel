#include "gfx/UIShader.h"

#include <GL/glew.h>

UIShader::UIShader()
    : Shader()
{
    Initialize();
}

UIShader::~UIShader() {}

void UIShader::Initialize()
{
    mShaderProgram = CreateShaderProgram("../shaders/common.vert", "../shaders/ui.frag");
    mLocModel = glGetUniformLocation(mShaderProgram, "modelMatrix");
    mLocView = glGetUniformLocation(mShaderProgram, "viewMatrix");
    mLocProj = glGetUniformLocation(mShaderProgram, "projectionMatrix");
    mLocObjectColor = glGetUniformLocation(mShaderProgram, "objectColor");
    mLocUseTexture = glGetUniformLocation(mShaderProgram, "useTexture");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "diffuseTexture");
    mLocDamageVignette = glGetUniformLocation(mShaderProgram, "damageVignette");
    mLocFlashStrength = glGetUniformLocation(mShaderProgram, "flashStrength");
    mLocFlashColor = glGetUniformLocation(mShaderProgram, "flashColor");
    mLocPanelSize = glGetUniformLocation(mShaderProgram, "panelSize");
    mLocBorderWidth = glGetUniformLocation(mShaderProgram, "borderWidth");
    mLocBorderColor = glGetUniformLocation(mShaderProgram, "borderColor");
    mLocCornerRadius = glGetUniformLocation(mShaderProgram, "cornerRadius");
    mLocGrayscale = glGetUniformLocation(mShaderProgram, "grayscale");
    mLocConvertSrgbToLinear = glGetUniformLocation(mShaderProgram, "convertSrgbToLinear");
}
