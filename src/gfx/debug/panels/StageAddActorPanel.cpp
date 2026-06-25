#include "gfx/debug/panels/StageAddActorPanel.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Planet.h"
#include "imgui.h"
#include "system/ActorLoadSystem.h"

#include <fstream>
#include <iostream>
#include <vector>

StageAddActorPanel::StageAddActorPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void StageAddActorPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        return;
    }

    if (ImGui::TreeNode("惑星追加")) {
        const char* planetModelLabels[] = {"通常惑星", "赤い惑星", "地形付き惑星"};
        const char* planetModels[] = {"planet.obj", "planet_2.obj", "planet_3.obj"};

        ImGui::Combo("惑星モデル", &mSelectedPlanetModelIndex, planetModelLabels, IM_ARRAYSIZE(planetModelLabels));

        if (ImGui::Button("惑星を追加")) {
            AddPlanetFromEditor(planetModels[mSelectedPlanetModelIndex]);
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("敵追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、敵を追加できません");
            ImGui::TreePop();
            return;
        }

        DrawPlanetCombo("敵の追加先惑星", mSelectedEnemyPlanetIndex);

        const char* enemyTypeLabels[] = {"通常敵", "ボス敵", "動かない敵", "動かない大きい敵"};
        const char* enemyTypes[] = {"normal", "boss", "normalFixed", "bigFixed"};

        ImGui::Combo("敵タイプ", &mSelectedEnemyTypeIndex, enemyTypeLabels, IM_ARRAYSIZE(enemyTypeLabels));

        const bool canAddEnemy = mSelectedEnemyPlanetIndex >= 0;

        if (!canAddEnemy) {
            ImGui::Text("敵を追加するには、追加先の惑星を選択してください");
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("敵を追加")) {
            AddEnemyFromEditor(enemyTypes[mSelectedEnemyTypeIndex], mSelectedEnemyPlanetIndex);
        }

        if (!canAddEnemy) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("足場追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、足場を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("追加先の惑星##platform", mSelectedPlatformPlanetIndex);

            const char* platformModelLabels[] = {"通常足場", "カーブ足場", "細い足場"};
            const char* platformModels[] = {"platform.obj", "curvePlatform.obj", "platform_thin.obj"};

            ImGui::Combo("モデル##platform", &mSelectedPlatformModelIndex, platformModelLabels,
                         IM_ARRAYSIZE(platformModelLabels));

            ImGui::SliderFloat("スケールX##platform", &mPlatformScale.x, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールY##platform", &mPlatformScale.y, 0.1f, 30.0f, "%.2f");
            ImGui::SliderFloat("スケールZ##platform", &mPlatformScale.z, 0.1f, 30.0f, "%.2f");

            const bool canAddPlatform = mSelectedPlatformPlanetIndex >= 0;

            if (!canAddPlatform) {
                ImGui::Text("足場を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("足場を追加")) {
                AddPlatformFromEditor(mSelectedPlatformPlanetIndex, platformModels[mSelectedPlatformModelIndex],
                                      mPlatformScale);
            }

            if (!canAddPlatform) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("クリスタル追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、クリスタルを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("クリスタルの追加先惑星", mSelectedCrystalPlanetIndex);

            const char* crystalTypeLabels[] = {"小さいクリスタル", "大きいクリスタル"};
            const char* crystalTypes[] = {"little", "big"};

            ImGui::Combo("クリスタルタイプ", &mSelectedCrystalTypeIndex, crystalTypeLabels,
                         IM_ARRAYSIZE(crystalTypeLabels));

            const bool canAddCrystal = mSelectedCrystalPlanetIndex >= 0;

            if (!canAddCrystal) {
                ImGui::Text("クリスタルを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("クリスタルを追加")) {
                AddCrystalFromEditor(crystalTypes[mSelectedCrystalTypeIndex], mSelectedCrystalPlanetIndex);
            }

            if (!canAddCrystal) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("NPC追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、NPCを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("NPCの追加先惑星", mSelectedNPCPlanetIndex);

            const char* npcTypeLabels[] = {"宇宙スライム", "母スライム", "プレイヤー型", "悪い母スライム",
                                           "博士スライム"};

            const char* npcTypes[] = {"spaceSlime", "motherSlime", "player", "badMotherSlime", "doctorSlime"};

            ImGui::Combo("NPCタイプ", &mSelectedNPCTypeIndex, npcTypeLabels, IM_ARRAYSIZE(npcTypeLabels));

            const bool canAddNPC = mSelectedNPCPlanetIndex >= 0;

            if (!canAddNPC) {
                ImGui::Text("NPCを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("NPCを追加")) {
                AddNPCFromEditor(npcTypes[mSelectedNPCTypeIndex], mSelectedNPCPlanetIndex);
            }

            if (!canAddNPC) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボートパーツ追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートパーツを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートパーツの追加先惑星", mSelectedBoatPartsPlanetIndex);

            const char* boatPartsTypeLabels[] = {"パーツ1", "パーツ2", "パーツ3", "パーツ4", "パーツ5"};
            const char* boatPartsTypes[] = {"parts1", "parts2", "parts3", "parts4", "parts5"};

            ImGui::Combo("ボートパーツタイプ", &mSelectedBoatPartsTypeIndex, boatPartsTypeLabels,
                         IM_ARRAYSIZE(boatPartsTypeLabels));

            const bool canAddBoatParts = mSelectedBoatPartsPlanetIndex >= 0;

            if (!canAddBoatParts) {
                ImGui::Text("ボートパーツを追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートパーツを追加")) {
                AddBoatPartsFromEditor(boatPartsTypes[mSelectedBoatPartsTypeIndex], mSelectedBoatPartsPlanetIndex);
            }

            if (!canAddBoatParts) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("ボート追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、ボートを追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("ボートの開始惑星", mSelectedBoatStartPlanetIndex);
            DrawPlanetCombo("ボートの移動先惑星", mSelectedBoatDestPlanetIndex);

            ImGui::InputInt("移動先ステージ", &mSelectedBoatDestStage);

            const bool canAddBoat = mSelectedBoatStartPlanetIndex >= 0 && mSelectedBoatDestPlanetIndex >= 0;

            if (!canAddBoat) {
                ImGui::Text("ボートを追加するには、開始惑星と移動先惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("ボートを追加")) {
                AddBoatFromEditor(mSelectedBoatStartPlanetIndex, mSelectedBoatDestPlanetIndex, mSelectedBoatDestStage);
            }

            if (!canAddBoat) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }

    if (ImGui::TreeNode("星追加")) {
        const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

        if (planets.empty()) {
            ImGui::Text("惑星が存在しないため、星を追加できません");
            ImGui::TreePop();
        } else {
            DrawPlanetCombo("星の追加先惑星", mSelectedStarPlanetIndex);

            const bool canAddStar = mSelectedStarPlanetIndex >= 0;

            if (!canAddStar) {
                ImGui::Text("星を追加するには、追加先の惑星を選択してください");
                ImGui::BeginDisabled();
            }

            if (ImGui::Button("星を追加")) {
                AddStarFromEditor(mSelectedStarPlanetIndex);
            }

            if (!canAddStar) {
                ImGui::EndDisabled();
            }

            ImGui::TreePop();
        }
    }
}

void StageAddActorPanel::DrawPlanetCombo(const char* label, int& selectedPlanetIndex)
{
    if (!mContext.game || !mContext.game->GetCurrentStage()) {
        selectedPlanetIndex = -1;
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (selectedPlanetIndex >= static_cast<int>(planets.size())) {
        selectedPlanetIndex = -1;
    }

    std::string previewText = "未選択";
    if (selectedPlanetIndex >= 0) {
        previewText = "惑星 " + std::to_string(selectedPlanetIndex);
    }

    if (ImGui::BeginCombo(label, previewText.c_str())) {
        for (int i = 0; i < static_cast<int>(planets.size()); ++i) {
            Planet* planet = planets[i];
            if (!planet) {
                continue;
            }

            std::string itemLabel = "惑星 " + std::to_string(i);
            bool isSelected = selectedPlanetIndex == i;

            if (ImGui::Selectable(itemLabel.c_str(), isSelected)) {
                selectedPlanetIndex = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void StageAddActorPanel::AddPlatformFromEditor(int currentPlanetNum, const std::string& modelPath,
                                               const glm::vec3& scale)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["platforms"]) {
        config["platforms"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["platforms"].size());

    YAML::Node platformNode;

    platformNode["currentPlanetNum"] = currentPlanetNum;
    platformNode["theta"] = 0.0f;
    platformNode["phi"] = 0.0f;
    platformNode["height"] = 1.0f;

    platformNode["facingYaw"] = 0.0f;

    platformNode["rotation"][0] = 0.0f;
    platformNode["rotation"][1] = 0.0f;
    platformNode["rotation"][2] = 0.0f;

    platformNode["scale"][0] = scale.x;
    platformNode["scale"][1] = scale.y;
    platformNode["scale"][2] = scale.z;

    platformNode["modelPath"] = modelPath;

    config["platforms"].push_back(platformNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreatePlatformFromStageNode(platformNode, index);
}

void StageAddActorPanel::AddPlanetFromEditor(const std::string& modelPath)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["planets"]) {
        config["planets"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int planetIndex = static_cast<int>(config["planets"].size());

    YAML::Node planetNode;

    planetNode["center"][0] = static_cast<float>(planetIndex) * 32.0f;
    planetNode["center"][1] = 0.0f;
    planetNode["center"][2] = 0.0f;

    planetNode["scale"][0] = 4.0f;
    planetNode["scale"][1] = 4.0f;
    planetNode["scale"][2] = 4.0f;

    planetNode["color"][0] = 1.0f;
    planetNode["color"][1] = 1.0f;
    planetNode["color"][2] = 1.0f;
    planetNode["color"][3] = 1.0f;

    planetNode["model"] = modelPath;
    planetNode["shape"] = "Sphere";
    planetNode["stageNum"] = planetIndex;
    planetNode["rocketSpawnCondition"] = "";

    config["planets"].push_back(planetNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreatePlanetFromStageNode(planetNode);
}

void StageAddActorPanel::AddEnemyFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["enemies"]) {
        config["enemies"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["enemies"].size());

    YAML::Node enemyNode;
    enemyNode["editorName"] = type == "boss" ? "新しいボス敵" : "新しい通常敵";
    enemyNode["type"] = type;
    enemyNode["currentPlanetNum"] = currentPlanetNum;
    enemyNode["theta"] = 0.0f;
    enemyNode["phi"] = 0.0f;
    enemyNode["height"] = 1.0f;

    const Planet* planet = planets[currentPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    enemyNode["pos"][0] = initialDistance;
    enemyNode["pos"][1] = 0.0f;
    enemyNode["pos"][2] = 0.0f;

    config["enemies"].push_back(enemyNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateEnemyFromStageNode(enemyNode, index);
}

void StageAddActorPanel::AddNPCFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["NPCs"]) {
        config["NPCs"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["NPCs"].size());

    YAML::Node npcNode;
    npcNode["type"] = type;
    npcNode["currentPlanetNum"] = currentPlanetNum;
    npcNode["theta"] = 0.0f;
    npcNode["phi"] = 0.0f;
    npcNode["height"] = 1.0f;
    npcNode["facingYaw"] = 0.0f;
    npcNode["radius"] = 0.75f;
    npcNode["name"] = "新しいNPC";
    npcNode["talkTexts"].push_back("こんにちは");

    config["NPCs"].push_back(npcNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateNPCFromStageNode(npcNode, index);
}

void StageAddActorPanel::AddCrystalFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["crystals"]) {
        config["crystals"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["crystals"].size());

    YAML::Node crystalNode;
    crystalNode["type"] = type;
    crystalNode["currentPlanetNum"] = currentPlanetNum;
    crystalNode["theta"] = 0.0f;
    crystalNode["phi"] = 0.0f;
    crystalNode["height"] = 1.0f;

    config["crystals"].push_back(crystalNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateCrystalFromStageNode(crystalNode, index);
}

void StageAddActorPanel::AddBoatPartsFromEditor(const std::string& type, int currentPlanetNum)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["boatParts"]) {
        config["boatParts"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["boatParts"].size());

    YAML::Node partNode;
    partNode["type"] = type;
    partNode["currentPlanetNum"] = currentPlanetNum;
    partNode["theta"] = 0.0f;
    partNode["phi"] = 0.0f;
    partNode["height"] = 1.0f;

    config["boatParts"].push_back(partNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatPartsFromStageNode(partNode, index);
}

void StageAddActorPanel::AddBoatFromEditor(int startPlanetNum, int destPlanetNum, int destStage)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (startPlanetNum < 0 || startPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid start planet index: " << startPlanetNum << std::endl;
        return;
    }

    if (destPlanetNum < 0 || destPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid destination planet index: " << destPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["boats"]) {
        config["boats"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["boats"].size());

    YAML::Node boatNode;
    boatNode["startPlanet"] = startPlanetNum;
    boatNode["destPlanet"] = destPlanetNum;
    boatNode["destStage"] = destStage;

    boatNode["theta"] = 0.0f;
    boatNode["phi"] = 0.0f;
    boatNode["height"] = 1.0f;
    boatNode["facingYaw"] = 0.0f;

    const Planet* planet = planets[startPlanetNum];
    const float initialHeight = 1.0f;
    const float initialDistance = planet ? planet->GetRadius() + initialHeight : 1.0f;

    boatNode["pos"][0] = initialDistance;
    boatNode["pos"][1] = 0.0f;
    boatNode["pos"][2] = 0.0f;

    config["boats"].push_back(boatNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateBoatFromStageNode(boatNode, index);
}

void StageAddActorPanel::AddStarFromEditor(int currentPlanetNum)
{
    if (!mContext.game || !mContext.game->GetCurrentStage() || !mContext.game->GetActorLoadSystem()) {
        return;
    }

    const auto& planets = mContext.game->GetCurrentStage()->GetPlanets();

    if (currentPlanetNum < 0 || currentPlanetNum >= static_cast<int>(planets.size())) {
        std::cerr << "Invalid planet index: " << currentPlanetNum << std::endl;
        return;
    }

    const std::string filePath = mContext.game->GetCurrentStageYamlPath();

    YAML::Node config;

    try {
        config = YAML::LoadFile(filePath);
    } catch (const YAML::Exception& e) {
        std::cerr << "Failed to load stage yaml: " << filePath << std::endl;
        std::cerr << e.what() << std::endl;
        return;
    }

    if (!config["star"]) {
        config["star"] = YAML::Node(YAML::NodeType::Sequence);
    }

    const int index = static_cast<int>(config["star"].size());

    YAML::Node starNode;
    starNode["currentPlanetNum"] = currentPlanetNum;
    starNode["theta"] = 0.0f;
    starNode["phi"] = 0.0f;
    starNode["height"] = 1.0f;
    starNode["isActive"] = true;

    config["star"].push_back(starNode);

    if (!SaveYamlFile(filePath, config)) {
        return;
    }

    mContext.game->GetActorLoadSystem()->CreateStarFromStageNode(starNode, index);
}

bool StageAddActorPanel::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}