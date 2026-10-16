#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

enum class ParticleBlendMode {
    Alpha,
    Additive,
};

enum class ParticleRenderMode {
    Billboard,

    // 画面上の速度方向へ向け、速度とvelocityStretchに応じて伸ばす。
    VelocityAligned,
};

// Fixed・ConeはParticleSpawnContext::direction、Hemisphereはnormalを基準に放出する。
enum class ParticleDirectionMode {
    Fixed,
    Sphere,
    Hemisphere,
    Cone,
};

// 粒子生成時に一様乱数で選ぶ範囲。maxがmin以下の場合はminを使う。
struct ParticleFloatRange {
    float min = 0.0f;
    float max = 0.0f;
};

// 1回の放出で生成する粒子の設定。範囲値は粒子ごとに選び、色とサイズは寿命に沿って線形補間する。
struct ParticleEmitterDefinition {
    std::string texturePath;

    ParticleBlendMode blendMode = ParticleBlendMode::Additive;
    ParticleRenderMode renderMode = ParticleRenderMode::Billboard;
    ParticleDirectionMode directionMode = ParticleDirectionMode::Sphere;

    int count = 1;

    // lifetimeは秒、speedはワールド単位/秒。startSizeは描画サイズの初期値。
    ParticleFloatRange lifetime{0.1f, 0.1f};
    ParticleFloatRange speed{0.0f, 0.0f};
    ParticleFloatRange startSize{0.1f, 0.1f};

    // 初期回転は度、角速度は度/秒。生成時にラジアンへ変換する。
    ParticleFloatRange rotationDegrees{0.0f, 0.0f};
    ParticleFloatRange angularVelocityDegrees{0.0f, 0.0f};

    // 終了サイズは、生成時に決まったstartSizeにこの倍率を掛ける。
    float endSizeMultiplier = 0.0f;

    // Coneの軸からの最大偏角。放出時に0〜180度へ制限する。
    float spreadAngleDegrees = 180.0f;

    // gravityは-normal方向の加速度。dragは速度にexp(-drag * 経過秒数)を掛ける減衰係数。
    float gravity = 0.0f;
    float drag = 0.0f;
    float velocityStretch = 0.0f;

    // ワールド軸に沿うオフセット。放出方向による回転は適用しない。
    glm::vec3 positionOffset{0.0f};

    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};
};

// エフェクトを構成するエミッター群。放出時はすべてのエミッターを使用する。
struct ParticleEffectDefinition {
    std::string displayName;
    std::vector<ParticleEmitterDefinition> emitters;
};

// 放出位置・方向はワールド座標。normalは半球放出の基準と重力の逆方向を兼ねる。
// directionは固定・円錐放出の軸で、ゼロベクトルならnormalを使う。
struct ParticleSpawnContext {
    glm::vec3 position{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec3 direction{0.0f, 0.0f, 1.0f};

    // 位置オフセット・初速度・初期サイズに掛ける倍率。放出時に負値は0として扱う。
    float scale = 1.0f;
};

// 生成済みの粒子状態。位置・速度・重力方向はワールド座標で、ageとlifetimeは秒単位。
struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec3 gravityDirection{0.0f, -1.0f, 0.0f};

    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};

    std::string texturePath;

    ParticleBlendMode blendMode = ParticleBlendMode::Additive;
    ParticleRenderMode renderMode = ParticleRenderMode::Billboard;

    float startSize = 0.1f;
    float endSize = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;

    // 回転はラジアン、角速度はラジアン/秒。定義側の度単位とは異なる。
    float rotationRadians = 0.0f;
    float angularVelocityRadians = 0.0f;
    float gravity = 0.0f;
    float drag = 0.0f;
    float velocityStretch = 0.0f;
};
