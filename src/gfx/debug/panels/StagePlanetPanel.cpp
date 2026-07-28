#include "gfx/debug/panels/StagePlanetPanel.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "Stage.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/BoatParts.h"
#include "actor/Crystal.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "gfx/debug/stage/StageYamlRepository.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

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

StagePlanetPanel::StagePlanetPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void StagePlanetPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (!ImGui::TreeNode("惑星")) {
        return;
    }

    ImGui::Separator();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const std::string treeLabel = "惑星 " + std::to_string(i) + "##planet" + std::to_string(i);

        if (ImGui::TreeNode(treeLabel.c_str())) {
            glm::vec3 center = planet->GetPos();
            const glm::vec3 previousScale = planet->GetScale();
            glm::vec3 scale = previousScale;

            bool centerChanged = false;
            bool scaleChanged = false;

            centerChanged |= ImGui::SliderFloat(("中心X##planetCenterX" + std::to_string(i)).c_str(), &center.x,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Y##planetCenterY" + std::to_string(i)).c_str(), &center.y,
                                                -100.0f, 100.0f, "%.2f");

            centerChanged |= ImGui::SliderFloat(("中心Z##planetCenterZ" + std::to_string(i)).c_str(), &center.z,
                                                -100.0f, 100.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールX##planetScaleX" + std::to_string(i)).c_str(), &scale.x, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールY##planetScaleY" + std::to_string(i)).c_str(), &scale.y, 0.1f,
                                               30.0f, "%.2f");

            scaleChanged |= ImGui::SliderFloat(("スケールZ##planetScaleZ" + std::to_string(i)).c_str(), &scale.z, 0.1f,
                                               30.0f, "%.2f");

            if (centerChanged) {
                planet->SetPos(center);
                UpdateActorsOnPlanetSurface(planet);
            }

            if (scaleChanged) {
                bool isSphere = false;

                scale.x = std::round(scale.x * 100.0f) / 100.0f;
                scale.y = std::round(scale.y * 100.0f) / 100.0f;
                scale.z = std::round(scale.z * 100.0f) / 100.0f;

                planet->SetTextureTiling(
                    glm::vec2(
                        std::max(1.0f, std::sqrt(std::abs(scale.x * scale.z))),
                        std::max(1.0f, std::abs(scale.y))));

                if (scale.x == scale.y && scale.y == scale.z && scale.x == scale.z) {
                    isSphere = true;
                }

                planet->SetScale(scale);

                if (isSphere) {
                    planet->SetPlanetShape("Sphere");
                } else {
                    planet->SetPlanetShape("Ellipse");
                }

                planet->SetRadius(scale.x);

                UpdateActorsOnPlanetSurface(planet);
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

            if (ImGui::Combo(("モデル##planetModel" + std::to_string(i)).c_str(), &selectedModelIndex,
                             planetModelLabels, IM_ARRAYSIZE(planetModelLabels))) {
                planet->SetModelPath(planetModels[selectedModelIndex]);

                if (mContext.game->GetMeshLoadSystem()) {
                    mContext.game->GetMeshLoadSystem()->SetActorMesh(planet);
                }
            }

            DrawTexturePicker(planet, i);
            DrawTextureTilingEditor(planet, i);

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
            const std::string currentSpawnCondition =
                planet->GetRocketSpawnCondition();
            int spawnConditionIndex = 0;
            for (int conditionIndex = 0;
                 conditionIndex < IM_ARRAYSIZE(spawnConditionValues);
                 ++conditionIndex) {
                if (currentSpawnCondition ==
                    spawnConditionValues[conditionIndex]) {
                    spawnConditionIndex = conditionIndex;
                    break;
                }
            }
            if (ImGui::Combo(
                    ("出現条件##rocketSpawnCondition" + std::to_string(i)).c_str(),
                    &spawnConditionIndex,
                    spawnConditionLabels,
                    IM_ARRAYSIZE(spawnConditionLabels))) {
                planet->SetRocketSpawnCondition(
                    spawnConditionValues[spawnConditionIndex]);
            }

            ImGui::TreePop();
        }
    }

    ImGui::TreePop();
}

void StagePlanetPanel::Save()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    YAML::Node config;

    if (!StageYamlRepository::LoadCurrentStage(mContext, config)) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    for (std::size_t i = 0; i < planets.size(); ++i) {
        Planet* planet = planets[i];
        if (!planet) {
            continue;
        }

        const glm::vec3 center = planet->GetPos();
        const glm::vec3 scale = planet->GetScale();

        config["planets"][i]["center"][0] = center.x;
        config["planets"][i]["center"][1] = center.y;
        config["planets"][i]["center"][2] = center.z;

        config["planets"][i]["scale"][0] = scale.x;
        config["planets"][i]["scale"][1] = scale.y;
        config["planets"][i]["scale"][2] = scale.z;

        config["planets"][i]["model"] = planet->GetModelPath();

        const std::string& textureOverride = planet->GetTextureOverridePath();
        if (textureOverride.empty()) {
            config["planets"][i].remove("textureOverride");
        } else {
            config["planets"][i]["textureOverride"] = textureOverride;
        }

        const glm::vec2 textureTiling = planet->GetTextureTiling();
        config["planets"][i]["textureTiling"][0] = textureTiling.x;
        config["planets"][i]["textureTiling"][1] = textureTiling.y;
        config["planets"][i]["rocketSpawnCondition"] =
            planet->GetRocketSpawnCondition();
    }

    StageYamlRepository::SaveCurrentStage(mContext, config);
}

void StagePlanetPanel::DrawTexturePicker(Planet* planet, std::size_t planetIndex)
{
    if (!planet) {
        return;
    }

    if (!mTextureAssetsScanned) {
        RefreshTextureAssets();
    }

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = planet->GetTextureOverridePath();
    ImGui::TextWrapped(
        "選択中: %s",
        selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());

    const std::string pickerId = "##planetTexturePicker" + std::to_string(planetIndex);
    if (ImGui::TreeNode(("テクスチャを選ぶ" + pickerId).c_str())) {
        const std::string filterId = "##planetTextureFilter" + std::to_string(planetIndex);
        ImGui::InputTextWithHint(
            filterId.c_str(),
            "ファイル名で検索",
            mTextureAssetFilter.data(),
            mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button(("更新##planetTextureRefresh" + std::to_string(planetIndex)).c_str())) {
            RefreshTextureAssets();
        }

        if (ImGui::Selectable(
                ("モデル標準に戻す##planetTextureDefault" + std::to_string(planetIndex)).c_str(),
                selectedTexture.empty())) {
            planet->SetTextureOverridePath("");
        }

        const std::string assetListId = "PlanetTextureAssetPicker##" + std::to_string(planetIndex);
        ImGui::BeginChild(assetListId.c_str(), ImVec2(0.0f, 180.0f), true);
        const std::string filter = ToLower(mTextureAssetFilter.data());
        for (const std::string& asset : mTextureAssets) {
            if (!filter.empty() && ToLower(asset).find(filter) == std::string::npos) {
                continue;
            }

            if (ImGui::Selectable(asset.c_str(), selectedTexture == asset)) {
                planet->SetTextureOverridePath(asset);
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

void StagePlanetPanel::DrawTextureTilingEditor(Planet* planet, std::size_t planetIndex)
{
    if (!planet) {
        return;
    }

    ImGui::SeparatorText("テクスチャ繰り返し");
    glm::vec2 textureTiling = planet->GetTextureTiling();
    const std::string tilingId = "UV繰り返し##planetTextureTiling" + std::to_string(planetIndex);
    if (ImGui::DragFloat2(
            tilingId.c_str(),
            &textureTiling.x,
            0.1f,
            0.01f,
            100.0f,
            "%.2f")) {
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

    ImGui::TextDisabled(
        "スケール変更時に自動追従します。横はX/Zの一周方向、縦はY方向です。");
}

void StagePlanetPanel::RefreshTextureAssets()
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

void StagePlanetPanel::UpdateActorsOnPlanetSurface(Planet* planet)
{
    if (!planet || !mContext.game) {
        return;
    }

    auto updateActor = [planet](Actor* actor) {
        if (!actor) {
            return;
        }

        const glm::vec3 newPos = planet->CalculateSurfacePos(actor->GetTheta(), actor->GetPhi(), actor->GetHeight());

        actor->SetPos(newPos);
    };

    if (!mContext.game->GetPlayers().empty()) {
        updateActor(mContext.game->GetPlayers()[0]);
    }

    for (Enemy* enemy : planet->GetEnemies()) {
        updateActor(enemy);
    }

    for (Crystal* crystal : planet->GetCrystals()) {
        updateActor(crystal);
    }

    for (Boat* boat : planet->GetBoats()) {
        updateActor(boat);
    }

    for (BoatParts* part : planet->GetBoatParts()) {
        updateActor(part);
    }

    for (NPC* npc : planet->GetNPCs()) {
        updateActor(npc);
    }

    if (Key* key = planet->GetKey()) {
        updateActor(key);
    }

    if (Star* star = planet->GetStar()) {
        updateActor(star);
    }
}
