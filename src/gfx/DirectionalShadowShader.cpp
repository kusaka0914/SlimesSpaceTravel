#include "gfx/DirectionalShadowShader.h"

#include <GL/glew.h>

DirectionalShadowShader::DirectionalShadowShader()
{
    mShaderProgram = CreateShaderProgram("../shaders/directionalShadow.vert", "../shaders/directionalShadow.frag");
    mLocModel = glGetUniformLocation(mShaderProgram, "modelMatrix");
    mLocLightSpaceMatrix = glGetUniformLocation(mShaderProgram, "lightSpaceMatrix");
    mLocUseSkinning = glGetUniformLocation(mShaderProgram, "useSkinning");
    mLocUseInstancing = glGetUniformLocation(mShaderProgram, "useInstancing");
    mLocBoneTransforms = glGetUniformLocation(mShaderProgram, "boneTransforms[0]");
}
