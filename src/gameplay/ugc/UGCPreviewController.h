#pragma once

#include "gameplay/ugc/UGCPreviewState.h"

#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Actor;
class Game;
class MeshLoadSystem;

// プレビューの表示設定と配置用Actorを管理する。Actorは本クラスが所有し、GameとMeshLoadSystemは所有しない。
class UGCPreviewController {
public:
    explicit UGCPreviewController(Game* actorOwner);
    ~UGCPreviewController();

    // 描画サイズをピクセル単位で指定し、幅320〜2048・高さ180〜1152に制限する。
    void SetRenderSize(int width, int height);
    void SetMeshLoadSystem(MeshLoadSystem& meshLoadSystem);
    int GetRenderWidth() const;
    int GetRenderHeight() const;

    void AdjustYaw(float yawDeltaRadians);
    float GetYaw() const;
    void ToggleVerticalView();
    bool IsViewedFromBelow() const;

    // deltaTimeは秒単位。編集レイヤーの高さへ追従を進め、更新後のワールドY座標を返す。
    float UpdateFocusY(float deltaTime);
    float GetFocusY() const;
    void SetEditLayer(int gridLayer);
    int GetEditLayer() const;

    void SetGridSize(float gridSize);
    float GetGridSize() const;
    void SetOrthographicHalfHeight(float halfHeight);
    float GetOrthographicHalfHeight() const;

    void SetPlatformPlacementPosition(const std::optional<glm::vec3>& position);
    const std::optional<glm::vec3>& GetPlatformPlacementPosition() const;
    void SetMovingPlatformPath(const std::optional<glm::vec3>& startPosition,
                               const std::optional<glm::vec3>& destinationPosition);
    const std::optional<glm::vec3>& GetMovingPlatformPathStart() const;
    const std::optional<glm::vec3>& GetMovingPlatformPathDestination() const;

    // positionが未指定またはmodelPathが空なら配置用Actorを破棄する。モデル変更時はActorを作り直す。
    void SetPlacementModel(const std::optional<glm::vec3>& position, const std::string& modelPath,
                           const glm::vec3& scale, const std::string& textureOverridePath);

    // 1つの配置用Actorを複数位置で描画するための位置一覧を保持する。空ならActorを破棄する。
    void SetPlacementModelPositions(const std::vector<glm::vec3>& positions, const std::string& modelPath,
                                    const glm::vec3& scale, const std::string& textureOverridePath);

    // 所有権を移さず配置用Actorを返す。未設定時はnullptr。モデルの変更・解除で無効になる。
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
