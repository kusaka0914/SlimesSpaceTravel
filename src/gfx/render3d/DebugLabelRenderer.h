#pragma once

#include <glm/glm.hpp>
#include <string>

class Actor;
class Renderer3D;

class DebugLabelRenderer {
public:
    explicit DebugLabelRenderer(const Renderer3D* renderer);

    void DrawDebugLabels(const glm::mat4& viewMat) const;

private:
    void DrawDebugLabel(const glm::mat4& viewMat, const Actor* actor, const std::string& label) const;

private:
    const Renderer3D* mRenderer;
};
