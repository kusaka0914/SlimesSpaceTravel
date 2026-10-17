#include "gfx/PlanetAtmosphereShader.h"

#include <GL/glew.h>

PlanetAtmosphereShader::PlanetAtmosphereShader()
{
    mShaderProgram = CreateShaderProgram("../shaders/planetAtmosphere.vert", "../shaders/planetAtmosphere.frag");
    mLocModel = glGetUniformLocation(mShaderProgram, "modelMatrix");
    mLocView = glGetUniformLocation(mShaderProgram, "viewMatrix");
    mLocProj = glGetUniformLocation(mShaderProgram, "projectionMatrix");
    mLocCameraWorldPosition = glGetUniformLocation(mShaderProgram, "cameraWorldPosition");
    mLocAtmosphereColor = glGetUniformLocation(mShaderProgram, "atmosphereColor");
    mLocAtmosphereStrength = glGetUniformLocation(mShaderProgram, "atmosphereStrength");
    mLocAtmospherePower = glGetUniformLocation(mShaderProgram, "atmospherePower");
    mLocSunDirection = glGetUniformLocation(mShaderProgram, "sunDirection");
}
