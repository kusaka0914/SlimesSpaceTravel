#include "editor/stage/panels/StagePlanetPanel.h"

#include "utils/StringUtils.h"

// GLEWはGLFWなどが取り込むOpenGLヘッダより先に読み込む。
#include <GL/glew.h>

#include "actor/Planet.h"
#include "app/Game.h"
#include "editor/assets/EditorAssetCatalog.h"
#include "editor/assets/EditorAssetDragDrop.h"
#include "gfx/Renderer3D.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"
#include "system/StageActorPlanetBindingService.h"
#include "world/Stage.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

StagePlanetPanel::StagePlanetPanel(DebugEditorContext& context, std::function<bool()> saveEditorTransforms)
    : DebugPanel(context),
      mSaveEditorTransforms(std::move(saveEditorTransforms))
{
}

void StagePlanetPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (mFocusedPlanet) {
        ImGui::SetNextItemOpen(true, ImGuiCond_Always);
    }
    if (!ImGui::TreeNode("惑星")) {
        return;
    }

    ImGui::Separator();

    for (std::size_t index = 0; index < planets.size(); ++index) {
        Planet* planet = planets[index];
        if (!planet || (mFocusedPlanet && planet != mFocusedPlanet)) {
            continue;
        }

        const std::string treeLabel = "惑星 " + std::to_string(index) + "##planet" + std::to_string(index);

        if (mFocusedPlanet) {
            ImGui::SetNextItemOpen(true, ImGuiCond_Always);
        }
        if (ImGui::TreeNode(treeLabel.c_str())) {
            DrawPlanetProperties(planet, index);
            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
    if (!mTransformSaveError.empty()) {
        ImGui::TextWrapped("%s", mTransformSaveError.c_str());
        if (ImGui::Button("惑星配置の保存を再試行")) {
            SaveEditorAuthoredTransforms();
        }
    }
}

void StagePlanetPanel::DrawSelectedPlanet(Planet* selectedPlanet)
{
    if (!selectedPlanet) {
        return;
    }

    mFocusedPlanet = selectedPlanet;
    Draw();
    mFocusedPlanet = nullptr;
}

bool StagePlanetPanel::SaveEditorAuthoredTransforms()
{
    if (!mSaveEditorTransforms || !mSaveEditorTransforms()) {
        mTransformSaveError = "編集した配置を保存できませんでした。編集内容は画面上に残っています。";
        return false;
    }
    mTransformSaveError.clear();
    return true;
}

void StagePlanetPanel::DrawTexturePicker(Planet* planet, std::size_t planetIndex)
{
    if (!planet) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = planet->GetTextureOverridePath();
    ImGui::TextWrapped("選択中: %s", selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());
    ImGui::Button(("画像アセットをここへドロップ##planetTextureDrop" + std::to_string(planetIndex)).c_str(),
                  ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Texture, droppedTexturePath)) {
        planet->SetTextureOverridePath(droppedTexturePath);
        if (mContext.game && mContext.game->GetRenderer3D() &&
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(droppedTexturePath) == 0) {
            mTextureAssetStatus = "テクスチャの読み込みに失敗しました: " + droppedTexturePath;
        } else {
            mTextureAssetStatus.clear();
        }
    }

    const std::string pickerId = "##planetTexturePicker" + std::to_string(planetIndex);
    if (ImGui::TreeNode(("テクスチャを選ぶ" + pickerId).c_str())) {
        const std::string filterId = "##planetTextureFilter" + std::to_string(planetIndex);
        ImGui::InputTextWithHint(filterId.c_str(), "ファイル名で検索", mTextureAssetFilter.data(),
                                 mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button(("更新##planetTextureRefresh" + std::to_string(planetIndex)).c_str())) {
            mContext.assetCatalog->Refresh();
        }

        if (ImGui::Selectable(("モデル標準に戻す##planetTextureDefault" + std::to_string(planetIndex)).c_str(),
                              selectedTexture.empty())) {
            planet->SetTextureOverridePath("");
        }

        const std::string assetListId = "PlanetTextureAssetPicker##" + std::to_string(planetIndex);
        ImGui::BeginChild(assetListId.c_str(), ImVec2(0.0f, 180.0f), true);
        const std::string filter = StringUtils::ToLower(mTextureAssetFilter.data());
        for (const std::string& asset : mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
            if (!filter.empty() && StringUtils::ToLower(asset).find(filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(asset.c_str(), selectedTexture == asset)) {
                planet->SetTextureOverridePath(asset);
                if (mContext.game && mContext.game->GetRenderer3D()) {
                    const GLuint texture = mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(asset);
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
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "%s", mTextureAssetStatus.c_str());
        }

        ImGui::TreePop();
    }

    if (!selectedTexture.empty() && mContext.game && mContext.game->GetRenderer3D()) {
        const GLuint texture = mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(selectedTexture);
        if (texture != 0) {
            ImGui::TextUnformatted("プレビュー");
            ImGui::Image(static_cast<ImTextureID>(texture), ImVec2(128.0f, 128.0f), ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f));
        }
    }
}

void StagePlanetPanel::DrawBackTexturePicker(Planet* planet, std::size_t planetIndex)
{
    if (!planet) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    ImGui::SeparatorText("楕円の裏側テクスチャ");
    ImGui::TextDisabled("惑星中心より下半分に使用します。未設定なら表側と同じテクスチャです。");

    const std::string& selectedTexture = planet->GetBackTextureOverridePath();
    ImGui::TextWrapped("選択中: %s", selectedTexture.empty() ? "表側と同じ" : selectedTexture.c_str());
    ImGui::Button(("画像アセットをここへドロップ##planetBackTextureDrop" + std::to_string(planetIndex)).c_str(),
                  ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Texture, droppedTexturePath)) {
        planet->SetBackTextureOverridePath(droppedTexturePath);
        if (mContext.game && mContext.game->GetRenderer3D() &&
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(droppedTexturePath) == 0) {
            mTextureAssetStatus = "裏側テクスチャの読み込みに失敗しました: " + droppedTexturePath;
        } else {
            mTextureAssetStatus.clear();
        }
    }

    const std::string pickerId = "##planetBackTexturePicker" + std::to_string(planetIndex);
    if (ImGui::TreeNode(("裏側テクスチャを選ぶ" + pickerId).c_str())) {
        const std::string filterId = "##planetBackTextureFilter" + std::to_string(planetIndex);
        ImGui::InputTextWithHint(filterId.c_str(), "ファイル名で検索", mTextureAssetFilter.data(),
                                 mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button(("更新##planetBackTextureRefresh" + std::to_string(planetIndex)).c_str())) {
            mContext.assetCatalog->Refresh();
        }

        if (ImGui::Selectable(("表側と同じ##planetBackTextureDefault" + std::to_string(planetIndex)).c_str(),
                              selectedTexture.empty())) {
            planet->SetBackTextureOverridePath("");
        }

        const std::string assetListId = "PlanetBackTextureAssetPicker##" + std::to_string(planetIndex);
        ImGui::BeginChild(assetListId.c_str(), ImVec2(0.0f, 180.0f), true);
        const std::string filter = StringUtils::ToLower(mTextureAssetFilter.data());
        for (const std::string& asset : mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
            if (!filter.empty() && StringUtils::ToLower(asset).find(filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(asset.c_str(), selectedTexture == asset)) {
                planet->SetBackTextureOverridePath(asset);
                if (mContext.game && mContext.game->GetRenderer3D()) {
                    const GLuint texture = mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(asset);
                    if (texture == 0) {
                        mTextureAssetStatus = "裏側テクスチャの読み込みに失敗しました: " + asset;
                    } else {
                        mTextureAssetStatus.clear();
                    }
                }
            }
        }
        ImGui::EndChild();

        if (!mTextureAssetStatus.empty()) {
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.25f, 1.0f), "%s", mTextureAssetStatus.c_str());
        }
        ImGui::TreePop();
    }

    float blendWidth = planet->GetTextureSideBlendWidth();
    if (ImGui::SliderFloat(("表裏の境界ぼかし##planetTextureSideBlendWidth" + std::to_string(planetIndex)).c_str(),
                           &blendWidth, 0.0f, 0.5f, "%.3f")) {
        planet->SetTextureSideBlendWidth(blendWidth);
    }

    if (!selectedTexture.empty() && mContext.game && mContext.game->GetRenderer3D()) {
        const GLuint texture = mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(selectedTexture);
        if (texture != 0) {
            ImGui::TextUnformatted("裏側プレビュー");
            ImGui::Image(static_cast<ImTextureID>(texture), ImVec2(128.0f, 128.0f), ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f));
        }
    }
}

void StagePlanetPanel::DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex)
{
    if (!planet) {
        return;
    }

    ImGui::SeparatorText("テクスチャ繰り返し");
    glm::vec2 textureTiling = planet->GetTextureTiling();
    const std::string tilingId = "UV繰り返し##planetTextureTiling" + std::to_string(planetIndex);
    if (ImGui::DragFloat2(tilingId.c_str(), &textureTiling.x, 0.1f, 0.01f, 100.0f, "%.2f")) {
        planet->SetTextureTiling(glm::max(textureTiling, glm::vec2(0.01f)));
    }

    if (ImGui::Button(("2回繰り返す##planetTextureTiling2" + std::to_string(planetIndex)).c_str())) {
        planet->SetTextureTiling(glm::vec2(2.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button(("4回繰り返す##planetTextureTiling4" + std::to_string(planetIndex)).c_str())) {
        planet->SetTextureTiling(glm::vec2(4.0f));
    }
    ImGui::SameLine();
    if (ImGui::Button(("1に戻す##planetTextureTilingReset" + std::to_string(planetIndex)).c_str())) {
        planet->SetTextureTiling(glm::vec2(1.0f));
    }
}

void StagePlanetPanel::DrawPlanetProperties(Planet* planet, std::size_t planetIndex)
{
    glm::vec3 center = planet->GetPos();
    const glm::vec3 previousCenter = center;
    const glm::vec3 previousScale = planet->GetScale();
    glm::vec3 scale = previousScale;

    bool centerChanged = false;
    bool scaleChanged = false;

    centerChanged |= ImGui::SliderFloat(("中心X##planetCenterX" + std::to_string(planetIndex)).c_str(), &center.x,
                                        -100.0f, 100.0f, "%.2f");

    centerChanged |= ImGui::SliderFloat(("中心Y##planetCenterY" + std::to_string(planetIndex)).c_str(), &center.y,
                                        -100.0f, 100.0f, "%.2f");

    centerChanged |= ImGui::SliderFloat(("中心Z##planetCenterZ" + std::to_string(planetIndex)).c_str(), &center.z,
                                        -100.0f, 100.0f, "%.2f");

    scaleChanged |= ImGui::SliderFloat(("スケールX##planetScaleX" + std::to_string(planetIndex)).c_str(), &scale.x,
                                       0.1f, 30.0f, "%.2f");

    scaleChanged |= ImGui::SliderFloat(("スケールY##planetScaleY" + std::to_string(planetIndex)).c_str(), &scale.y,
                                       0.1f, 30.0f, "%.2f");

    scaleChanged |= ImGui::SliderFloat(("スケールZ##planetScaleZ" + std::to_string(planetIndex)).c_str(), &scale.z,
                                       0.1f, 30.0f, "%.2f");

    if (centerChanged) {
        planet->SetPos(center);
        if (mContext.planetMoveMode == PlanetMoveMode::WithBoundActors) {
            StageActorPlanetBindingService::TranslateActorsBoundToPlanet(planet, center - previousCenter);
        } else {
            StageActorPlanetBindingService::PreserveBoundActorWorldPositionsAfterPlanetMove(planet,
                                                                                            center - previousCenter);
        }
        planet->CaptureEditorAuthoredPosition();
        mHasPendingTransformEdit = true;
    }

    if (scaleChanged) {
        scale.x = std::round(scale.x * 100.0f) / 100.0f;
        scale.y = std::round(scale.y * 100.0f) / 100.0f;
        scale.z = std::round(scale.z * 100.0f) / 100.0f;

        planet->SetTextureTiling(
            glm::vec2(std::max(1.0f, std::sqrt(std::abs(scale.x * scale.z))), std::max(1.0f, std::abs(scale.y))));

        planet->SetScale(scale);

        planet->SetRadius(scale.x);

        StageActorPlanetBindingService::ReprojectSurfaceActorsAfterPlanetScale(planet);
        planet->CaptureEditorAuthoredScale();
        mHasPendingTransformEdit = true;
    }

    // スライダー操作中は実行時の見た目だけ更新し、操作終了後に編集済みTransformをまとめて保存する。
    if (mHasPendingTransformEdit && !ImGui::IsAnyItemActive()) {
        SaveEditorAuthoredTransforms();
        mHasPendingTransformEdit = false;
    }

    const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
    const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

    std::string currentModel = planet->GetModelPath();
    int selectedModelIndex = 0;

    for (int modelIndex = 0; modelIndex < IM_ARRAYSIZE(planetModels); ++modelIndex) {
        if (currentModel == planetModels[modelIndex]) {
            selectedModelIndex = modelIndex;
            break;
        }
    }

    if (ImGui::Combo(("モデル##planetModel" + std::to_string(planetIndex)).c_str(), &selectedModelIndex,
                     planetModelLabels, IM_ARRAYSIZE(planetModelLabels))) {
        planet->SetModelPath(planetModels[selectedModelIndex]);

        if (mContext.game->GetMeshLoadSystem()) {
            mContext.game->GetMeshLoadSystem()->SetActorMesh(planet);
        }
    }
    ImGui::Button(("モデルアセットをここへドロップ##planetModelDrop" + std::to_string(planetIndex)).c_str(),
                  ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Model, droppedModelPath)) {
        planet->SetModelPath(droppedModelPath);
        if (mContext.game->GetMeshLoadSystem()) {
            mContext.game->GetMeshLoadSystem()->SetActorMesh(planet);
        }
    }
    ImGui::TextWrapped("選択中: %s", planet->GetModelPath().c_str());

    DrawTexturePicker(planet, planetIndex);
    if (planet->GetPlanetShape() == Planet::PlanetShape::Ellipse) {
        DrawBackTexturePicker(planet, planetIndex);
    }
    DrawTextureTilingEditor(planet, planetIndex);

    ImGui::SeparatorText("近接時の重力");
    bool canAttractNearbyPlayer = planet->CanAttractNearbyPlayer();
    if (ImGui::Checkbox(("近づいたプレイヤーを吸い付ける##planetNearbyGravity" + std::to_string(planetIndex)).c_str(),
                        &canAttractNearbyPlayer)) {
        planet->SetCanAttractNearbyPlayer(canAttractNearbyPlayer);
    }

    bool shouldReactToOverheadGravityRay = planet->ShouldReactToOverheadGravityRay();
    if (ImGui::Checkbox(("頭上重力レイに反応する##planetOverheadGravityRay" + std::to_string(planetIndex)).c_str(),
                        &shouldReactToOverheadGravityRay)) {
        planet->SetShouldReactToOverheadGravityRay(shouldReactToOverheadGravityRay);
    }

    ImGui::SeparatorText("ロケット出現条件");
    const char* spawnConditionLabels[] = {
        "なし",
        "敵をすべて倒す",
        "ボートパーツをすべて集める",
    };
    const char* spawnConditionValues[] = {
        "",
        "AllEnemiesDead",
        "AllBoatPartsCollected",
    };
    const std::string currentSpawnCondition = planet->GetRocketSpawnCondition();
    int spawnConditionIndex = 0;
    for (int conditionIndex = 0; conditionIndex < IM_ARRAYSIZE(spawnConditionValues); ++conditionIndex) {
        if (currentSpawnCondition == spawnConditionValues[conditionIndex]) {
            spawnConditionIndex = conditionIndex;
            break;
        }
    }
    if (ImGui::Combo(("出現条件##rocketSpawnCondition" + std::to_string(planetIndex)).c_str(), &spawnConditionIndex,
                     spawnConditionLabels, IM_ARRAYSIZE(spawnConditionLabels))) {
        planet->SetRocketSpawnCondition(spawnConditionValues[spawnConditionIndex]);
    }
}
