#include "Planet.h"

#include "actor/Boat.h"

#include <algorithm>
#include <cmath>

Planet::Planet(Game* game)
    : Actor(game),
      mStageNum(0),
      mColor(1.0f),
      mCurrentStage(nullptr)
{
}

namespace {
glm::vec2 ReadVec2(const YAML::Node& node, const char* key, const glm::vec2& defaultValue)
{
    if (!node[key] || !node[key].IsSequence() || node[key].size() < 2) {
        return defaultValue;
    }

    return glm::vec2(node[key][0] ? node[key][0].as<float>() : defaultValue.x,
                     node[key][1] ? node[key][1].as<float>() : defaultValue.y);
}

glm::vec3 ReadVec3(const YAML::Node& node, const char* key, const glm::vec3& defaultValue)
{
    if (!node[key] || !node[key].IsSequence() || node[key].size() < 3) {
        return defaultValue;
    }

    return glm::vec3(node[key][0] ? node[key][0].as<float>() : defaultValue.x,
                     node[key][1] ? node[key][1].as<float>() : defaultValue.y,
                     node[key][2] ? node[key][2].as<float>() : defaultValue.z);
}

glm::vec4 ReadVec4(const YAML::Node& node, const char* key, const glm::vec4& defaultValue)
{
    if (!node[key] || !node[key].IsSequence() || node[key].size() < 4) {
        return defaultValue;
    }

    return glm::vec4(node[key][0] ? node[key][0].as<float>() : defaultValue.x,
                     node[key][1] ? node[key][1].as<float>() : defaultValue.y,
                     node[key][2] ? node[key][2].as<float>() : defaultValue.z,
                     node[key][3] ? node[key][3].as<float>() : defaultValue.w);
}

std::string ReadString(const YAML::Node& node, const char* key, const std::string& defaultValue)
{
    return node[key] ? node[key].as<std::string>() : defaultValue;
}

int ReadInt(const YAML::Node& node, const char* key, int defaultValue)
{
    return node[key] ? node[key].as<int>() : defaultValue;
}

int FindShortestScaleAxisIndex(const glm::vec3& absoluteScale)
{
    int shortestAxisIndex = 0;
    if (absoluteScale.y < absoluteScale[shortestAxisIndex]) {
        shortestAxisIndex = 1;
    }
    if (absoluteScale.z < absoluteScale[shortestAxisIndex]) {
        shortestAxisIndex = 2;
    }
    return shortestAxisIndex;
}
} // namespace

void Planet::ApplyConfig(const YAML::Node& node)
{
    const glm::vec3 center = ReadVec3(node, "center", glm::vec3(0.0f));
    SetPos(center);

    const glm::vec3 scale = ReadVec3(node, "scale", glm::vec3(1.0f));
    SetScale(scale);
    SetRadius(scale.x);

    const glm::vec4 color = ReadVec4(node, "color", glm::vec4(1.0f));
    SetColor(color);

    const std::string modelPath = ReadString(node, "model", "planet.obj");
    SetModelPath(modelPath);

    const std::string textureOverride = ReadString(node, "textureOverride", "");
    SetTextureOverridePath(textureOverride);
    SetBackTextureOverridePath(
        ReadString(node, "backTextureOverride", ""));
    SetTextureSideBlendWidth(
        node["textureSideBlendWidth"]
            ? node["textureSideBlendWidth"].as<float>()
            : 0.05f);
    SetCanAttractNearbyPlayer(
        node["canAttractNearbyPlayer"]
            ? node["canAttractNearbyPlayer"].as<bool>()
            : true);
    SetShouldReactToOverheadGravityRay(
        node["reactsToOverheadGravityRay"]
            ? node["reactsToOverheadGravityRay"].as<bool>()
            : false);

    const glm::vec2 automaticTextureTiling(
        std::max(1.0f, std::sqrt(std::abs(scale.x * scale.z))),
        std::max(1.0f, std::abs(scale.y)));
    const glm::vec2 textureTiling =
        ReadVec2(node, "textureTiling", automaticTextureTiling);
    SetTextureTiling(glm::max(textureTiling, glm::vec2(0.01f)));

    const int stageNum = ReadInt(node, "stageNum", 0);
    SetStageNum(stageNum);

    const std::string rocketSpawnCondition = ReadString(node, "rocketSpawnCondition", "");
    SetRocketSpawnCondition(rocketSpawnCondition);
}

void Planet::Initialize()
{
    mProgressController.Initialize(mActorRegistry);
}

void Planet::OnEnemyDead()
{
    mProgressController.OnEnemyDead(mActorRegistry);
}

void Planet::OnBoatPartsObtained()
{
    mProgressController.OnBoatPartsObtained(mActorRegistry);
}

Planet::PlanetShape Planet::GetPlanetShape() const
{
    constexpr float scaleComparisonEpsilon = 0.0001f;

    const glm::vec3 absoluteScale = glm::abs(GetScale());
    const bool hasNoScale =
        absoluteScale.x <= scaleComparisonEpsilon &&
        absoluteScale.y <= scaleComparisonEpsilon &&
        absoluteScale.z <= scaleComparisonEpsilon;
    if (hasNoScale) {
        return PlanetShape::Normal;
    }

    const bool hasUniformScale =
        std::abs(absoluteScale.x - absoluteScale.y) <=
            scaleComparisonEpsilon &&
        std::abs(absoluteScale.y - absoluteScale.z) <=
            scaleComparisonEpsilon;
    return hasUniformScale
        ? PlanetShape::Sphere
        : PlanetShape::Ellipse;
}

glm::vec3 Planet::CalculateEllipseVerticalDirection(
    const glm::vec3& worldPosition) const
{
    const glm::vec3 absoluteScale = glm::abs(GetScale());
    const int verticalAxisIndex =
        FindShortestScaleAxisIndex(absoluteScale);

    glm::vec3 verticalAxis(0.0f);
    verticalAxis[verticalAxisIndex] = 1.0f;

    constexpr float centerPlaneEpsilon = 0.000001f;
    const float verticalOffset =
        glm::dot(worldPosition - GetPos(), verticalAxis);
    return verticalOffset < -centerPlaneEpsilon
        ? -verticalAxis
        : verticalAxis;
}

Planet::EllipseSurfaceFace Planet::ResolveEllipseSurfaceFace(
    const glm::vec3& worldPosition) const
{
    constexpr float minimumAxisRadius = 0.001f;
    constexpr float sideRegionHalfWidthRatio = 0.2f;

    const glm::vec3 absoluteScale = glm::abs(GetScale());
    const int verticalAxisIndex =
        FindShortestScaleAxisIndex(absoluteScale);
    const float verticalAxisRadius =
        std::max(
            absoluteScale[verticalAxisIndex],
            minimumAxisRadius);
    const float normalizedVerticalOffset =
        (worldPosition[verticalAxisIndex] -
         GetPos()[verticalAxisIndex]) /
        verticalAxisRadius;

    if (normalizedVerticalOffset >
        sideRegionHalfWidthRatio) {
        return EllipseSurfaceFace::Front;
    }
    if (normalizedVerticalOffset <
        -sideRegionHalfWidthRatio) {
        return EllipseSurfaceFace::Back;
    }
    return EllipseSurfaceFace::Side;
}

Planet::EllipseSurfaceFace
Planet::ResolveEllipseSurfaceHemisphere(
    const glm::vec3& worldPosition) const
{
    const glm::vec3 absoluteScale = glm::abs(GetScale());
    const int verticalAxisIndex =
        FindShortestScaleAxisIndex(absoluteScale);
    const float verticalOffset =
        worldPosition[verticalAxisIndex] -
        GetPos()[verticalAxisIndex];
    return verticalOffset >= 0.0f
        ? EllipseSurfaceFace::Front
        : EllipseSurfaceFace::Back;
}

bool Planet::ArePositionsOnSameSurfaceFace(
    const glm::vec3& firstWorldPosition,
    const glm::vec3& secondWorldPosition) const
{
    if (GetPlanetShape() != PlanetShape::Ellipse) {
        return true;
    }

    // The side region is still used to keep enemy movement away from an
    // ellipse edge. Interaction filtering only needs to prevent attacks and
    // targeting through the planet, so positions on the same hemisphere stay
    // related even when knockback moves one actor into the side region.
    return ResolveEllipseSurfaceHemisphere(
               firstWorldPosition) ==
           ResolveEllipseSurfaceHemisphere(
               secondWorldPosition);
}

Planet::EllipseSurfaceProjection
Planet::CalculateEllipseSurfaceProjection(
    const glm::vec3& worldPosition) const
{
    constexpr float minimumRadius = 0.001f;
    constexpr float positionEpsilon = 0.000001f;
    constexpr int closestPointIterations = 48;

    const glm::vec3 radii = glm::max(
        glm::abs(GetScale()),
        glm::vec3(minimumRadius));
    const glm::vec3 localPosition = worldPosition - GetPos();
    const glm::vec3 squaredRadii = radii * radii;

    const float scaledDistanceSquared =
        localPosition.x * localPosition.x / squaredRadii.x +
        localPosition.y * localPosition.y / squaredRadii.y +
        localPosition.z * localPosition.z / squaredRadii.z;
    const bool isOutside = scaledDistanceSquared >= 1.0f;

    glm::vec3 surfaceLocalPosition(0.0f);
    if (glm::dot(localPosition, localPosition) <= positionEpsilon) {
        int shortestAxisIndex = 0;
        if (radii.y < radii[shortestAxisIndex]) {
            shortestAxisIndex = 1;
        }
        if (radii.z < radii[shortestAxisIndex]) {
            shortestAxisIndex = 2;
        }
        surfaceLocalPosition[shortestAxisIndex] =
            radii[shortestAxisIndex];
    } else if (!isOutside) {
        const float radialScale =
            1.0f / std::sqrt(
                std::max(scaledDistanceSquared, positionEpsilon));
        surfaceLocalPosition = localPosition * radialScale;
    } else {
        const auto calculateConstraint =
            [&localPosition, &squaredRadii](double lambda) {
                double constraint = 0.0;
                for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
                    const double radiusSquared =
                        static_cast<double>(squaredRadii[axisIndex]);
                    const double coordinate =
                        static_cast<double>(localPosition[axisIndex]);
                    const double denominator = lambda + radiusSquared;
                    constraint +=
                        radiusSquared * coordinate * coordinate /
                        (denominator * denominator);
                }
                return constraint;
            };

        double minimumLambda = 0.0;
        double maximumLambda = static_cast<double>(
            std::max({squaredRadii.x, squaredRadii.y, squaredRadii.z}));
        while (calculateConstraint(maximumLambda) > 1.0) {
            maximumLambda *= 2.0;
        }

        for (int iteration = 0;
             iteration < closestPointIterations;
             ++iteration) {
            const double middleLambda =
                (minimumLambda + maximumLambda) * 0.5;
            if (calculateConstraint(middleLambda) > 1.0) {
                minimumLambda = middleLambda;
            } else {
                maximumLambda = middleLambda;
            }
        }

        const double closestPointLambda = maximumLambda;
        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            const double radiusSquared =
                static_cast<double>(squaredRadii[axisIndex]);
            surfaceLocalPosition[axisIndex] =
                static_cast<float>(
                    radiusSquared *
                    static_cast<double>(localPosition[axisIndex]) /
                    (closestPointLambda + radiusSquared));
        }
    }

    glm::vec3 outwardNormal =
        surfaceLocalPosition / squaredRadii;
    if (glm::length(outwardNormal) <= positionEpsilon) {
        outwardNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        outwardNormal = glm::normalize(outwardNormal);
    }

    EllipseSurfaceProjection projection;
    projection.position = GetPos() + surfaceLocalPosition;
    projection.outwardNormal = outwardNormal;
    projection.distance =
        glm::length(worldPosition - projection.position);
    projection.isOutside = isOutside;
    return projection;
}

bool Planet::HasAppearedRocket() const
{
    for (const Boat* boat : mActorRegistry.GetBoats()) {
        if (boat && boat->GetIsActive() &&
            boat->HasAppeared()) {
            return true;
        }
    }
    return false;
}

glm::vec3 Planet::CalculateSurfacePos(float theta, float phi, float height) const
{
    const glm::vec3 dir =
        glm::normalize(glm::vec3(std::cos(phi) * std::cos(theta), std::sin(phi), std::cos(phi) * std::sin(theta)));

    return mPos + dir * (mRadius + height);
}
