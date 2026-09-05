#include "gfx/Shader3D.h"

#include <GL/glew.h>

Shader3D::Shader3D()
    : Shader()
{
    Initialize();
}

Shader3D::~Shader3D() = default;

void Shader3D::Initialize()
{
    mShaderProgram = CreateShaderProgram("../shaders/vertex.glsl", "../shaders/fragment.glsl");
    mLocModel = glGetUniformLocation(mShaderProgram, "model");
    mLocView = glGetUniformLocation(mShaderProgram, "view");
    mLocProj = glGetUniformLocation(mShaderProgram, "projection");
    mLocObjectColor = glGetUniformLocation(mShaderProgram, "objectColor");
    mLocUseTexture = glGetUniformLocation(mShaderProgram, "useTexture");
    mLocDiffuseTexture = glGetUniformLocation(mShaderProgram, "diffuseTexture");
    mLocSunDirection = glGetUniformLocation(mShaderProgram, "sunDirection");
    mLocSunColor = glGetUniformLocation(mShaderProgram, "sunColor");
    mLocSunIntensity = glGetUniformLocation(mShaderProgram, "sunIntensity");
    mLocEnvironmentColor = glGetUniformLocation(mShaderProgram, "environmentColor");
    mLocDayEnvironmentIntensity =
        glGetUniformLocation(mShaderProgram, "dayEnvironmentIntensity");
    mLocNightEnvironmentIntensity =
        glGetUniformLocation(mShaderProgram, "nightEnvironmentIntensity");
    mLocViewPos = glGetUniformLocation(mShaderProgram, "viewPos");
    mLocToonLevels = glGetUniformLocation(mShaderProgram, "toonLevels");
    mLocToonStrength = glGetUniformLocation(mShaderProgram, "toonStrength");
    mLocRimColor = glGetUniformLocation(mShaderProgram, "rimColor");
    mLocDayRimStrength = glGetUniformLocation(mShaderProgram, "dayRimStrength");
    mLocNightRimStrength = glGetUniformLocation(mShaderProgram, "nightRimStrength");
    mLocRimPower = glGetUniformLocation(mShaderProgram, "rimPower");
    mLocMaterialMinimumReflectance =
        glGetUniformLocation(mShaderProgram, "materialMinimumReflectance");
    mLocMaterialRimBoost =
        glGetUniformLocation(mShaderProgram, "materialRimBoost");
    mLocIsUnlit = glGetUniformLocation(mShaderProgram, "isUnlit");
    mLocUseSkinning = glGetUniformLocation(mShaderProgram, "useSkinning");
    mLocUseInstancing =
        glGetUniformLocation(mShaderProgram, "useInstancing");
    mLocBoneTransforms = glGetUniformLocation(mShaderProgram, "boneTransforms[0]");
    mLocTextureTiling = glGetUniformLocation(mShaderProgram, "textureTiling");
    mLocUseBackTexture =
        glGetUniformLocation(mShaderProgram, "useBackTexture");
    mLocBackTexture =
        glGetUniformLocation(mShaderProgram, "backTexture");
    mLocTextureSideBlendWidth =
        glGetUniformLocation(mShaderProgram, "textureSideBlendWidth");
    mLocColorMultiplier =
        glGetUniformLocation(mShaderProgram, "colorMultiplier");
    mLocApplyOutputGamma =
        glGetUniformLocation(mShaderProgram, "applyOutputGamma");
    mLocEmissiveColor =
        glGetUniformLocation(mShaderProgram, "emissiveColor");
    mLocEmissiveIntensity =
        glGetUniformLocation(mShaderProgram, "emissiveIntensity");
    mLocLightSpaceMatrix =
        glGetUniformLocation(mShaderProgram, "lightSpaceMatrix");
    mLocShadowMap = glGetUniformLocation(mShaderProgram, "shadowMap");
    mLocShadowsEnabled =
        glGetUniformLocation(mShaderProgram, "shadowsEnabled");
    mLocShadowMinimumBias =
        glGetUniformLocation(mShaderProgram, "shadowMinimumBias");
    mLocShadowMaximumBias =
        glGetUniformLocation(mShaderProgram, "shadowMaximumBias");
}
