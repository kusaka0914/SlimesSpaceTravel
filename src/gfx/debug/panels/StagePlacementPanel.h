#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/stage/StageSelectionController.h"

#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

class Actor;

class StagePlacementPanel : public DebugPanel {
public:
    using Callback = std::function<void()>;

    StagePlacementPanel(
        DebugEditorContext& context,
        StageSelectionController& selectionController,
        Callback pushUndoCallback = {});

    void Draw() override;
    void DrawObjectList();
    void DrawPlayerSpawn();
    void Save();

    void RequestOpenPickedActorPlacement();

private:
    struct ActorGroup {
        std::string label;
        std::string sequenceName;
        std::vector<StageActorInstance> actors;
    };

    std::vector<ActorGroup> CollectActorGroups() const;

    void DrawPlayerSpawnEditor();
    bool SavePlayerSpawnFromCurrentTransform(class Player* player);
    void DrawActorList(const ActorGroup& group);
    void DrawSelectedActorEditor();
    void DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    bool DrawPlatformTypeEditor(
        class Platform* platform,
        const std::string& sequenceName,
        std::size_t listIndex);
    void DrawPlatformBehaviorEditors(class Platform* platform, int yamlIndex);
    bool ChangePlatformType(
        const std::string& sourceSequenceName,
        std::size_t sourceIndex,
        const struct PlatformTypeDefinition& targetType);
    void DrawPlacementModelPicker(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    void DrawNPCModelPicker(class NPC* npc, const std::string& sequenceName, std::size_t listIndex);
    void DrawBoatModelPicker(class Boat* boat, const std::string& sequenceName, std::size_t listIndex);
    void DrawTextureOverrideEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex);
    void RefreshTextureAssets();

    void SaveActorsYaml(YAML::Node& config, const ActorGroup& group);
    void SaveActorCommonYaml(YAML::Node& config, const std::string& sequenceName, Actor* actor);

    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const;
    void ApplyActorEditorRotation(Actor* actor);

    void RebuildPhysicsWorldIfNeeded(bool required);

private:
    StageSelectionController& mSelectionController;
    Callback mPushUndoCallback;
    bool mRequestOpenPickedActorPlacement = false;
    int mSelectedSpawnPlayerIndex = 0;
    std::string mPlayerSpawnStatus;
    std::array<char, 128> mTextureAssetFilter = {};
    std::array<char, 128> mPlacementModelAssetFilter = {};
    std::array<char, 128> mNPCModelAssetFilter = {};
    std::array<char, 128> mBoatModelAssetFilter = {};
    std::vector<std::string> mTextureAssets;
    std::string mTextureAssetStatus;
    std::string mRubyGenerationStatus;
    std::string mPlatformTypeChangeStatus;
    bool mTextureAssetsScanned = false;
};
