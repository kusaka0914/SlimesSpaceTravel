#include "UIShader.h"
#include <GL/glew.h>

UIShader::UIShader() : Shader()
{
    Initialize();
}

UIShader::~UIShader() {}

void UIShader::Initialize()
{
    mShaderProgram = CreateShaderProgram("../shaders/vertex.glsl", "../shaders/UIfragment.glsl");
    mLocModel = glGetUniformLocation(mShaderProgram, "model");
    mLocView = glGetUniformLocation(mShaderProgram, "view");
    mLocProj = glGetUniformLocation(mShaderProgram, "projection");
    mLocObjectColor = glGetUniformLocation(mShaderProgram, "objectColor");
    mLocUseTexture = glGetUniformLocation(mShaderProgram, "useTexture");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "diffuseTexture");
    mLocConvertSrgbToLinear =
        glGetUniformLocation(mShaderProgram, "convertSrgbToLinear");
}
