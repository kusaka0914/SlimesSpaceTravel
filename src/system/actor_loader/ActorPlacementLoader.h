#pragma once

#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

class Actor;
class Planet;

// ステージYAMLの配置・回転・スケールをActorへ反映する。ActorやPlanetは所有しない。
class ActorPlacementLoader {
public:
    // posがあればPlanet中心からのローカル座標を優先し、なければtheta・phi（ラジアン）と高さから求める。
    glm::vec3 CalculatePos(const YAML::Node& node, const Planet& currentPlanet) const;

    // 配置と表示条件・重力設定を反映する。Actor・Planetがnullptrなら何もしない。
    void ApplyPlacementFromStageNode(Actor* actor, const YAML::Node& node, Planet* currentPlanet, int stageYamlIndex,
                                     float defaultHeight = 0.0f) const;

    // 向き・上方向・姿勢を反映する。rotationQuatが有効なら、その姿勢を最後に適用する。
    void ApplyRotationFromStageNode(Actor* actor, const YAML::Node& node) const;

    // 指定されたスケール・テクスチャ反復数・上書き画像を反映する。未指定の値は維持する。
    void ApplyScaleFromStageNode(Actor* actor, const YAML::Node& node) const;
};
