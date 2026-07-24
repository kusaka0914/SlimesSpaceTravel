#include "gfx/debug/panels/StagePlacementPanel.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/StageObject.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"
#include "system/PhysicsSystem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>

namespace {
std::string ToLower(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return value;
}

bool IsSupportedTextureExtension(const std::filesystem::path& path)
{
    const std::string extension = ToLower(path.extension().string());
    return extension == ".png" || extension == ".jpg" || extension == ".jpeg" ||
           extension == ".bmp" || extension == ".tga";
}
}

StagePlacementPanel::StagePlacementPanel(DebugEditorContext& context, StageSelectionController& selectionController)
    : DebugPanel(context),
      mSelectionController(selectionController)
{
}

void StagePlacementPanel::RequestOpenPickedActorPlacement()
{
    mRequestOpenPickedActorPlacement = true;
}

void StagePlacementPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (mRequestOpenPickedActorPlacement) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode("オブジェクト配置")) {
        return;
    }

    const std::vector<ActorGroup> groups = CollectActorGroups();

    ImGui::Separator();

    for (const ActorGroup& group : groups) {
        DrawActorList(group);
    }

    ImGui::TreePop();

    mRequestOpenPickedActorPlacement = false;
}

void StagePlacementPanel::Save()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    const std::vector<ActorGroup> groups = CollectActorGroups();

    for (const ActorGroup& group : groups) {
        SaveActorsYaml(config, group);
    }

    StageYamlRepository::SaveCurrentStage(mContext, config);
}

std::vector<StagePlacementPanel::ActorGroup> StagePlacementPanel::CollectActorGroups() const
{
    std::vector<ActorGroup> groups;

    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return groups;
    }

    for (const StageActorTypeInfo& info : StageActorQuery::GetTypeInfos()) {
        ActorGroup group;
        group.label = info.displayName;
        group.sequenceName = info.sequenceName;
        groups.emplace_back(group);
    }

    const std::vector<StageActorInstance> instances =
        StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

    for (const StageActorInstance& instance : instances) {
        if (!instance.actor) {
            continue;
        }

        for (ActorGroup& group : groups) {
            if (group.sequenceName != instance.ref.sequenceName) {
                continue;
            }

            group.actors.emplace_back(instance.actor);
            break;
        }
    }

    return groups;
}

void StagePlacementPanel::DrawActorList(const ActorGroup& group)
{
    const std::string treeLabel = group.label + "##" + group.sequenceName;

    const auto& pickedActorRef = mSelectionController.GetPickedActorRef();

    if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == group.sequenceName) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode(treeLabel.c_str())) {
        return;
    }

    if (group.actors.empty()) {
        ImGui::Text("なし");
        ImGui::TreePop();
        return;
    }

    for (std::size_t i = 0; i < group.actors.size(); ++i) {
        DrawActorPlacementEditor(group.actors[i], group.sequenceName, i);
    }

    ImGui::TreePop();
}

void StagePlacementPanel::DrawActorPlacementEditor(Actor* actor, const std::string& sequenceName, std::size_t listIndex)
{
    if (!actor) {
        return;
    }

    const int yamlIndex = actor->GetStageYamlIndex();

    std::string itemLabel =
        "index " + std::to_string(yamlIndex) + "##" + sequenceName + "_" + std::to_string(listIndex);

    const auto& pickedActorRef = mSelectionController.GetPickedActorRef();

    if (mRequestOpenPickedActorPlacement && pickedActorRef && pickedActorRef->sequenceName == sequenceName &&
        pickedActorRef->yamlIndex == yamlIndex) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }

    if (!ImGui::TreeNode(itemLabel.c_str())) {
        return;
    }

    if (StageObject* stageObject = dynamic_cast<StageObject*>(actor)) {
        ImGui::Text("モデル: %s", stageObject->GetModelPath().c_str());
        bool collisionEnabled = stageObject->GetCollisionEnabled();
        if (ImGui::Checkbox(
                ("当たり判定##stageObjectCollision" + std::to_string(yamlIndex)).c_str(),
                &collisionEnabled)) {
            stageObject->SetCollisionEnabled(collisionEnabled);
            RebuildPhysicsWorldIfNeeded(true);
        }
    }

    float theta = actor->GetTheta();
    float phi = actor->GetPhi();
    float height = actor->GetHeight();

    bool placementChanged = false;

    placementChanged |= ImGui::DragFloat(("theta##" + sequenceName + std::to_string(listIndex)).c_str(), &theta, 0.001f,
                                         -3.141593f, 3.141593f, "%.6f");

    placementChanged |= ImGui::DragFloat(("phi##" + sequenceName + std::to_string(listIndex)).c_str(), &phi, 0.001f,
                                         -1.570796f, 1.570796f, "%.6f");

    placementChanged |= ImGui::DragFloat(("height##" + sequenceName + std::to_string(listIndex)).c_str(), &height,
                                         0.01f, -10.0f, 10.0f, "%.3f");

    if (placementChanged) {
        theta = std::round(theta * 1000000.0f) / 1000000.0f;
        phi = std::round(phi * 1000000.0f) / 1000000.0f;
        height = std::round(height * 1000.0f) / 1000.0f;

        actor->SetSphericalPlacement(theta, phi, height);

        Planet* planet = actor->GetCurrentPlanet();
        if (planet) {
            actor->SetPos(planet->CalculateSurfacePos(theta, phi, height));
        }

        ApplyActorEditorRotation(actor);
    }

    bool posChanged = false;
    bool physicsRebuildRequired = false;

    Planet* planet = actor->GetCurrentPlanet();

    glm::vec3 localPos = actor->GetPos();
    if (planet) {
        localPos -= planet->GetPos();
    }

    posChanged |= ImGui::DragFloat(("posX##actorPosX" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.x,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    posChanged |= ImGui::DragFloat(("posY##actorPosY" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.y,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    posChanged |= ImGui::DragFloat(("posZ##actorPosZ" + sequenceName + std::to_string(listIndex)).c_str(), &localPos.z,
                                   0.01f, -100.0f, 100.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (posChanged) {
        localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
        localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
        localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

        const glm::vec3 worldPos = planet ? planet->GetPos() + localPos : localPos;
        actor->SetPos(worldPos);
    }

    glm::vec3 rotationRad = actor->GetEditorRotation();
    glm::vec3 rotationDeg = glm::degrees(rotationRad);

    bool rotationChanged = false;

    rotationChanged |= ImGui::DragFloat(("Pitch##actorPitch" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.x, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    rotationChanged |= ImGui::DragFloat(("Yaw##actorYaw" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.y, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    rotationChanged |= ImGui::DragFloat(("Roll##actorRoll" + sequenceName + std::to_string(listIndex)).c_str(),
                                        &rotationDeg.z, 0.1f, -180.0f, 180.0f, "%.1f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (rotationChanged) {
        rotationDeg.x = std::round(rotationDeg.x * 10.0f) / 10.0f;
        rotationDeg.y = std::round(rotationDeg.y * 10.0f) / 10.0f;
        rotationDeg.z = std::round(rotationDeg.z * 10.0f) / 10.0f;

        rotationRad = glm::radians(rotationDeg);

        actor->SetEditorRotation(rotationRad);
        ApplyActorEditorRotation(actor);
    }

    const bool canEditTextureTiling =
        dynamic_cast<Platform*>(actor) != nullptr ||
        dynamic_cast<StageObject*>(actor) != nullptr;

    const glm::vec3 previousScale = actor->GetScale();
    glm::vec3 scale = previousScale;

    bool scaleChanged = false;

    scaleChanged |= ImGui::DragFloat(("スケールX##actorScaleX" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.x, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    scaleChanged |= ImGui::DragFloat(("スケールY##actorScaleY" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.y, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    scaleChanged |= ImGui::DragFloat(("スケールZ##actorScaleZ" + sequenceName + std::to_string(listIndex)).c_str(),
                                     &scale.z, 0.01f, 0.01f, 30.0f, "%.2f");
    physicsRebuildRequired |= ImGui::IsItemDeactivatedAfterEdit();

    if (scaleChanged) {
        scale.x = std::round(scale.x * 100.0f) / 100.0f;
        scale.y = std::round(scale.y * 100.0f) / 100.0f;
        scale.z = std::round(scale.z * 100.0f) / 100.0f;

        actor->SetScale(scale);
        const bool horizontalScaleChanged =
            std::abs(scale.x - previousScale.x) > 0.0001f ||
            std::abs(scale.z - previousScale.z) > 0.0001f;
        if (canEditTextureTiling && horizontalScaleChanged) {
            actor->SetTextureTiling(
                glm::vec2(
                    std::max(1.0f, std::abs(scale.x)),
                    std::max(1.0f, std::abs(scale.z))));
        }
    }

    if (canEditTextureTiling) {
        DrawTextureOverrideEditor(actor, sequenceName, listIndex);

        ImGui::SeparatorText("テクスチャ繰り返し");

        glm::vec2 textureTiling = actor->GetTextureTiling();
        const std::string tilingId =
            "UV繰り返し##actorTextureTiling" + sequenceName + std::to_string(listIndex);
        if (ImGui::DragFloat2(
                tilingId.c_str(),
                &textureTiling.x,
                0.1f,
                0.01f,
                100.0f,
                "%.2f")) {
            textureTiling = glm::max(textureTiling, glm::vec2(0.01f));
            actor->SetTextureTiling(textureTiling);
        }

        const std::string autoTilingButtonId =
            "スケールX/Zから自動設定##" + sequenceName + std::to_string(listIndex);
        if (ImGui::Button(autoTilingButtonId.c_str())) {
            const glm::vec3 actorScale = actor->GetScale();
            actor->SetTextureTiling(
                glm::vec2(
                    std::max(1.0f, std::abs(actorScale.x)),
                    std::max(1.0f, std::abs(actorScale.z))));
        }
        ImGui::SameLine();
        if (ImGui::Button(
                ("1に戻す##textureTilingReset" + sequenceName + std::to_string(listIndex)).c_str())) {
            actor->SetTextureTiling(glm::vec2(1.0f));
        }

        ImGui::TextDisabled(
            "X/Zスケール変更時に自動追従します。手動で微調整することもできます。");
    }

    RebuildPhysicsWorldIfNeeded(physicsRebuildRequired);

    const glm::vec3 pos = actor->GetPos();
    ImGui::Text("pos: %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);

    ImGui::TreePop();
}

void StagePlacementPanel::DrawTextureOverrideEditor(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!actor) {
        return;
    }

    if (!mTextureAssetsScanned) {
        RefreshTextureAssets();
    }

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = actor->GetTextureOverridePath();
    ImGui::TextWrapped(
        "選択中: %s",
        selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());

    const std::string pickerId =
        "##actorTexturePicker" + sequenceName + std::to_string(listIndex);
    if (ImGui::TreeNode(("テクスチャを選ぶ" + pickerId).c_str())) {
        const std::string filterId =
            "##actorTextureFilter" + sequenceName + std::to_string(listIndex);
        ImGui::InputTextWithHint(
            filterId.c_str(),
            "ファイル名で検索",
            mTextureAssetFilter.data(),
            mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button(
                ("更新##actorTextureRefresh" + sequenceName + std::to_string(listIndex)).c_str())) {
            RefreshTextureAssets();
        }

        if (ImGui::Selectable(
                ("モデル標準に戻す##actorTextureDefault" + sequenceName + std::to_string(listIndex)).c_str(),
                selectedTexture.empty())) {
            actor->SetTextureOverridePath("");
        }

        const std::string assetListId =
            "ActorTextureAssetPicker##" + sequenceName + std::to_string(listIndex);
        ImGui::BeginChild(assetListId.c_str(), ImVec2(0.0f, 180.0f), true);
        const std::string filter = ToLower(mTextureAssetFilter.data());
        for (const std::string& asset : mTextureAssets) {
            if (!filter.empty() && ToLower(asset).find(filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(asset.c_str(), selectedTexture == asset)) {
                actor->SetTextureOverridePath(asset);
                if (mContext.game && mContext.game->GetRenderer3D()) {
                    const GLuint texture =
                        mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(asset);
                    if (texture == 0) {
                        mTextureAssetStatus = "テクスチャの読み込みに失敗しました: " + asset;
                    } else {
                        mTextureAssetStatus.clear();
                    }
                }
            }
        }
        ImGui::EndChild();

        if (!mTextureAssetStatus.empty()) {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.25f, 1.0f),
                "%s",
                mTextureAssetStatus.c_str());
        }

        ImGui::TreePop();
    }

    if (!selectedTexture.empty() && mContext.game && mContext.game->GetRenderer3D()) {
        const GLuint texture =
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(selectedTexture);
        if (texture != 0) {
            ImGui::TextUnformatted("プレビュー");
            ImGui::Image(
                static_cast<ImTextureID>(texture),
                ImVec2(128.0f, 128.0f),
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f));
        }
    }
}

void StagePlacementPanel::RefreshTextureAssets()
{
    mTextureAssets.clear();
    mTextureAssetsScanned = true;

    const std::filesystem::path assetsRoot("../assets");
    const std::filesystem::path textureRoot = assetsRoot / "textures";
    std::error_code error;
    if (!std::filesystem::is_directory(textureRoot, error)) {
        mTextureAssetStatus = "assets/textures が見つかりません";
        return;
    }

    for (std::filesystem::recursive_directory_iterator it(textureRoot, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error) || !IsSupportedTextureExtension(it->path())) {
            continue;
        }

        const std::filesystem::path relative =
            std::filesystem::relative(it->path(), assetsRoot, error);
        if (error) {
            error.clear();
            continue;
        }

        mTextureAssets.emplace_back(relative.generic_string());
    }

    std::sort(mTextureAssets.begin(), mTextureAssets.end());
    mTextureAssetStatus.clear();
}

void StagePlacementPanel::SaveActorsYaml(YAML::Node& config, const ActorGroup& group)
{
    for (Actor* actor : group.actors) {
        SaveActorCommonYaml(config, group.sequenceName, actor);
    }
}

void StagePlacementPanel::SaveActorCommonYaml(YAML::Node& config, const std::string& sequenceName, Actor* actor)
{
    if (!actor) {
        return;
    }

    const int index = actor->GetStageYamlIndex();
    if (index < 0) {
        return;
    }

    const std::size_t yamlIndex = static_cast<std::size_t>(index);

    if (!config[sequenceName] || !config[sequenceName].IsSequence()) {
        return;
    }

    if (yamlIndex >= config[sequenceName].size()) {
        return;
    }

    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "theta", actor->GetTheta());
    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "phi", actor->GetPhi());
    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "height", actor->GetHeight());

    glm::vec3 localPos = actor->GetPos();
    if (actor->GetCurrentPlanet()) {
        localPos -= actor->GetCurrentPlanet()->GetPos();
    }

    localPos.x = std::round(localPos.x * 100.0f) / 100.0f;
    localPos.y = std::round(localPos.y * 100.0f) / 100.0f;
    localPos.z = std::round(localPos.z * 100.0f) / 100.0f;

    StageYamlRepository::SetSequenceValue(config, sequenceName, yamlIndex, "pos", YAML::Node(YAML::NodeType::Sequence));
    config[sequenceName][yamlIndex]["pos"][0] = localPos.x;
    config[sequenceName][yamlIndex]["pos"][1] = localPos.y;
    config[sequenceName][yamlIndex]["pos"][2] = localPos.z;

    const glm::vec3 rotation = actor->GetEditorRotation();

    config[sequenceName][yamlIndex]["facingYaw"] = rotation.y;
    config[sequenceName][yamlIndex]["rotation"][0] = rotation.x;
    config[sequenceName][yamlIndex]["rotation"][1] = rotation.y;
    config[sequenceName][yamlIndex]["rotation"][2] = rotation.z;

    const glm::vec3 scale = actor->GetScale();

    config[sequenceName][yamlIndex]["scale"][0] = scale.x;
    config[sequenceName][yamlIndex]["scale"][1] = scale.y;
    config[sequenceName][yamlIndex]["scale"][2] = scale.z;

    const glm::vec3 upVec = actor->GetUpVec();

    config[sequenceName][yamlIndex]["upVec"][0] = upVec.x;
    config[sequenceName][yamlIndex]["upVec"][1] = upVec.y;
    config[sequenceName][yamlIndex]["upVec"][2] = upVec.z;

    if (const StageObject* stageObject = dynamic_cast<const StageObject*>(actor)) {
        config[sequenceName][yamlIndex]["modelPath"] = stageObject->GetModelPath();
        config[sequenceName][yamlIndex]["collision"] =
            stageObject->GetCollisionEnabled();
    }

    if (dynamic_cast<const Platform*>(actor) ||
        dynamic_cast<const StageObject*>(actor)) {
        const glm::vec2 textureTiling = actor->GetTextureTiling();
        config[sequenceName][yamlIndex]["textureTiling"][0] = textureTiling.x;
        config[sequenceName][yamlIndex]["textureTiling"][1] = textureTiling.y;

        const std::string& textureOverride = actor->GetTextureOverridePath();
        if (textureOverride.empty()) {
            config[sequenceName][yamlIndex].remove("textureOverride");
        } else {
            config[sequenceName][yamlIndex]["textureOverride"] = textureOverride;
        }
    }
}

glm::vec3 StagePlacementPanel::CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const
{
    if (!actor) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 baseUp(0.0f, 1.0f, 0.0f);

    Planet* planet = actor->GetCurrentPlanet();

    if (planet && planet->GetPlanetShape() == Planet::PlanetShape::Sphere) {
        glm::vec3 toActor = actor->GetPos() - planet->GetPos();

        if (glm::length(toActor) > 1e-6f) {
            baseUp = glm::normalize(toActor);
        }
    }

    glm::vec3 baseForward(0.0f, 0.0f, 1.0f);

    baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);

    if (glm::length(baseForward) < 1e-6f) {
        baseForward = glm::vec3(1.0f, 0.0f, 0.0f);
        baseForward = baseForward - baseUp * glm::dot(baseForward, baseUp);
    }

    baseForward = glm::normalize(baseForward);

    glm::vec3 baseRight = glm::normalize(glm::cross(baseForward, baseUp));

    const float pitch = rotationRad.x;
    const float yaw = rotationRad.y;
    const float roll = rotationRad.z;

    glm::mat4 rot(1.0f);
    rot = glm::rotate(rot, yaw, baseUp);
    rot = glm::rotate(rot, pitch, baseRight);
    rot = glm::rotate(rot, roll, baseForward);

    glm::vec3 upVec = glm::vec3(rot * glm::vec4(baseUp, 0.0f));

    if (glm::length(upVec) < 1e-6f) {
        return baseUp;
    }

    return glm::normalize(upVec);
}

void StagePlacementPanel::ApplyActorEditorRotation(Actor* actor)
{
    if (!actor) {
        return;
    }

    const glm::vec3 rotation = actor->GetEditorRotation();

    actor->SetFacingYaw(rotation.y);
    actor->SetUpVec(CalculateActorUpVecFromEditorRotation(actor, rotation));
}

void StagePlacementPanel::RebuildPhysicsWorldIfNeeded(bool required)
{
    if (!required || !mContext.game || !mContext.game->GetPhysicsSystem()) {
        return;
    }

    mContext.game->GetPhysicsSystem()->Initialize();
}
