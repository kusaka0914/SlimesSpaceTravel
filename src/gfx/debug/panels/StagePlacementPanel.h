#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Actor;

class StagePlacementPanel : public DebugPanel {
public:
    StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController);

    void Draw() override;
    void Save();

    void RequestOpenPickedActorPlacement();

private:
    struct ActorGroup {
        std::string label;
        std::string sequenceName;
        std::vector<Actor*> actors;
    };

    std::vector<ActorGroup> CollectActorGroups() const;

    void DrawActorList(const ActorGroup& group);
    void DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    void DrawTextureOverrideEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    void RefreshTextureAssets();

    void SaveActorsYaml(YAML::Node& config, const ActorGroup& group);
    void SaveActorCommonYaml(YAML::Node& config, const std::string& sequenceName, Actor* actor);

    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const;
    void ApplyActorEditorRotation(Actor* actor);

    void RebuildPhysicsWorldIfNeeded(bool required);

private:
    StageSelectionController& mSelectionController;
    bool mRequestOpenPickedActorPlacement = false;
    std::array<char, 128> mTextureAssetFilter = {};
    std::vector<std::string> mTextureAssets;
    std::string mTextureAssetStatus;
    bool mTextureAssetsScanned = false;
};
