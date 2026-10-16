#pragma once

#include "gfx/Shader.h"

class PlanetAtmosphereShader final : public Shader {
public:
    PlanetAtmosphereShader();

    int GetLocCameraWorldPosition() const { return mLocCameraWorldPosition; }
    int GetLocAtmosphereColor() const { return mLocAtmosphereColor; }
    int GetLocAtmosphereStrength() const { return mLocAtmosphereStrength; }
    int GetLocAtmospherePower() const { return mLocAtmospherePower; }
    int GetLocSunDirection() const { return mLocSunDirection; }

private:
    int mLocCameraWorldPosition = -1;
    int mLocAtmosphereColor = -1;
    int mLocAtmosphereStrength = -1;
    int mLocAtmospherePower = -1;
    int mLocSunDirection = -1;
};
