#include "gfx/DirectionalShadowShader.h"

#include <GL/glew.h>

DirectionalShadowShader::DirectionalShadowShader()
{
    mShaderProgram = CreateShaderProgram(
        "../shaders/directionalShadowVertex.glsl",
        "../shaders/directionalShadowFragment.glsl");
    mLocModel = glGetUniformLocation(mShaderProgram, "model");
    mLocLightSpaceMatrix =
        glGetUniformLocation(mShaderProgram, "lightSpaceMatrix");
    mLocUseSkinning =
        glGetUniformLocation(mShaderProgram, "useSkinning");
    mLocUseInstancing =
        glGetUniformLocation(mShaderProgram, "useInstancing");
    mLocBoneTransforms =
        glGetUniformLocation(mShaderProgram, "boneTransforms[0]");
}
