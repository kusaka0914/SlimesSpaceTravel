#pragma once

#include <glm/glm.hpp>

struct LightingSettings {
    glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.35f, 1.0f, -0.25f));
    glm::vec3 sunColor = glm::vec3(1.0f, 0.92f, 0.82f);
    float sunIntensity = 1.1f;

    glm::vec3 environmentColor = glm::vec3(0.48f, 0.58f, 0.78f);
    float dayEnvironmentIntensity = 0.22f;
    float nightEnvironmentIntensity = 0.72f;

    glm::vec3 rimColor = glm::vec3(0.42f, 0.72f, 1.0f);
    float dayRimStrength = 0.07f;
    float nightRimStrength = 0.20f;
    float rimPower = 2.5f;
};

struct DirectionalShadowSettings {
    int mapResolution = 2048;
    float projectionHalfExtent = 42.0f;
    float lightDistance = 55.0f;
    float nearPlane = 1.0f;
    float farPlane = 120.0f;
    float minimumBias = 0.0007f;
    float maximumBias = 0.004f;
};
