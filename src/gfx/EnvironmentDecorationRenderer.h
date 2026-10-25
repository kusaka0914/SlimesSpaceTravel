#pragma once

#include "actor/Planet.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

class DirectionalShadowShader;
class MeshLoadSystem;
struct LoadedModel;
class Shader3D;

// 惑星のバイオーム装飾を生成し、モデル別のインスタンス描画へまとめる。モデル・惑星は非所有参照。
class EnvironmentDecorationRenderer {
public:
    ~EnvironmentDecorationRenderer();

    // 惑星一覧・描画位置・スケール・モデル・装飾設定の変更時だけ、配置とGPUバッファを再生成する。
    // modelLoaderは呼出中だけ借用する。取得したモデルはShutdownまで有効に保つ。nullptrなら再生成時の装飾は空。
    void Prepare(const std::vector<Planet*>& planets, MeshLoadSystem* modelLoader);

    // Prepare済みの装飾を視点との距離で絞って描画する。shaderのプログラムを事前に有効化しておくこと。
    void Draw(Shader3D& shader, const glm::vec3& cameraPosition) const;

    // 影用の注目位置から近い装飾を描画する。深度パスとshaderは呼び出し側で開始しておくこと。
    void DrawShadow(DirectionalShadowShader& shader, const glm::vec3& shadowFocusPosition) const;

    // OpenGLコンテキストが有効な間にインスタンスバッファを解放し、惑星一覧とバッチを空にする。
    void Shutdown();

private:
    struct DecorationBatch {
        const LoadedModel* model = nullptr;
        unsigned int instanceBuffer = 0;
        std::vector<glm::mat4> transforms;
        glm::vec3 planetCenter{0.0f};
        float planetRadius = 0.0f;
        float maximumDrawDistance = 0.0f;
    };

    struct DecoratedPlanetIdentity {
        const Planet* planet = nullptr;
        glm::vec3 position{0.0f};
        glm::vec3 scale{0.0f};
        std::string modelPath;
        std::string texturePath;
        Planet::Biome biome = Planet::Biome::None;
        float decorationDensity = 0.0f;
    };

    bool HavePlanetsChanged(const std::vector<Planet*>& planets) const;
    void RebuildForPlanets(const std::vector<Planet*>& planets, MeshLoadSystem* modelLoader);
    void AppendPlanetBatches(Planet& planet, MeshLoadSystem& modelLoader);
    void ReleaseInstanceBuffers();
    void UploadBatch(DecorationBatch& batch);

    std::vector<DecoratedPlanetIdentity> mDecoratedPlanets;
    std::vector<DecorationBatch> mBatches;
};
