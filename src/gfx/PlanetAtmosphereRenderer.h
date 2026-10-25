#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Planet;
class PlanetAtmosphereShader;

// 大気殻の拡大倍率とFresnel指数。描画時の色・強度は各Planet::VisualSettingsを使う。
struct PlanetAtmosphereSettings {
    float fresnelPower = 3.2f;
    float shellScale = 1.025f;
};

// 惑星モデルを拡大した殻を加算合成し、大気の縁を描画する。惑星・メッシュは所有しない。
class PlanetAtmosphereRenderer {
public:
    PlanetAtmosphereRenderer();
    ~PlanetAtmosphereRenderer();

    // 各行列・視点・光の方向は3D描画と同じ座標系で渡す。無効な惑星やメッシュのない惑星は除外する。
    void Draw(const std::vector<Planet*>& planets, const glm::mat4& view, const glm::mat4& projection,
              const glm::vec3& cameraPosition, const glm::vec3& sunDirection) const;

    // OpenGLコンテキストが有効な間にシェーダーを解放する。
    void Shutdown();

    PlanetAtmosphereSettings& GetSettings() { return mSettings; }
    const PlanetAtmosphereSettings& GetSettings() const { return mSettings; }

private:
    std::unique_ptr<PlanetAtmosphereShader> mShader;
    PlanetAtmosphereSettings mSettings;
};
