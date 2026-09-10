#pragma once

#include <glm/glm.hpp>

#include <memory>
#include <vector>

class Planet;
class PlanetAtmosphereShader;

struct PlanetAtmosphereSettings {
    glm::vec3 color{0.28f, 0.62f, 1.0f};
    float strength = 0.38f;
    float fresnelPower = 3.2f;
    float shellScale = 1.025f;
};

class PlanetAtmosphereRenderer {
public:
    PlanetAtmosphereRenderer();
    ~PlanetAtmosphereRenderer();

    void Draw(
        const std::vector<Planet*>& planets,
        const glm::mat4& view,
        const glm::mat4& projection,
        const glm::vec3& cameraPosition,
        const glm::vec3& sunDirection) const;
    void Shutdown();

    PlanetAtmosphereSettings& GetSettings() { return mSettings; }
    const PlanetAtmosphereSettings& GetSettings() const
    {
        return mSettings;
    }

private:
    std::unique_ptr<PlanetAtmosphereShader> mShader;
    PlanetAtmosphereSettings mSettings;
};
