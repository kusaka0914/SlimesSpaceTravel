#pragma once

#include "gfx/debug/ugc/UGCPreviewState.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Actor;
class Game;
class MeshLoadSystem;

class UGCPreviewController {
public:
    explicit UGCPreviewController(Game* actorOwner);
    ~UGCPreviewController();

    void SetRenderSize(int width, int height);
    void SetMeshLoadSystem(MeshLoadSystem& meshLoadSystem);
    int GetRenderWidth() const;
    int GetRenderHeight() const;

    void AdjustYaw(float yawDeltaRadians);
    float GetYaw() const;
    void ToggleVerticalView();
    bool IsViewedFromBelow() const;
    float UpdateFocusY(float deltaTime);
    float GetFocusY() const;
    void SetEditLayer(int gridLayer);
    int GetEditLayer() const;

    void SetGridSize(float gridSize);
    float GetGridSize() const;
    void SetOrthographicHalfHeight(float halfHeight);
    float GetOrthographicHalfHeight() const;

    void SetPlatformPlacementPosition(
        const std::optional<glm::vec3>& position);
    const std::optional<glm::vec3>& GetPlatformPlacementPosition() const;
    void SetMovingPlatformPath(
        const std::optional<glm::vec3>& startPosition,
        const std::optional<glm::vec3>& destinationPosition);
    const std::optional<glm::vec3>& GetMovingPlatformPathStart() const;
    const std::optional<glm::vec3>& GetMovingPlatformPathDestination() const;

    void SetPlacementModel(
        const std::optional<glm::vec3>& position,
        const std::string& modelPath,
        const glm::vec3& scale,
        const std::string& textureOverridePath);
    void SetPlacementModelPositions(
        const std::vector<glm::vec3>& positions,
        const std::string& modelPath,
        const glm::vec3& scale,
        const std::string& textureOverridePath);
    Actor* GetPlacementModel() const;
    const std::vector<glm::vec3>& GetPlacementModelPositions() const;

private:
    Game* mActorOwner;
    MeshLoadSystem* mMeshLoadSystem = nullptr;
    UGCPreviewState mPreviewState;
    float mGridSize = 1.0f;
    float mOrthographicHalfHeight = 20.0f;
    std::optional<glm::vec3> mPlatformPlacementPosition;
    std::optional<glm::vec3> mMovingPlatformPathStart;
    std::optional<glm::vec3> mMovingPlatformPathDestination;
    std::unique_ptr<Actor> mPlacementModel;
    std::vector<glm::vec3> mPlacementModelPositions;
};
