#include "DebugUIRenderer.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Crystal.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/CameraSystem.h"

#include <string>
#include <vector>

DebugUIRenderer::DebugUIRenderer(Game* game)
    : mGame(game)
{
}

void DebugUIRenderer::Draw()
{
    ImGui::Begin("デバッグ");

    DrawPerformance();
    DrawPlayer();
    DrawCamera();
    // DrawStage1();
    // DrawDebugDrawSettings();

    ImGui::End();
}

void DebugUIRenderer::DrawPerformance()
{
    if (ImGui::CollapsingHeader("パフォーマンス")) {
        const float fps = ImGui::GetIO().Framerate;

        ImGui::Text("FPS: %.1f", fps);

        if (fps > 0.0f) {
            ImGui::Text("フレームタイム: %.3f ms", 1000.0f / fps);
        }
    }
}

void DebugUIRenderer::DrawPlayer()
{
    if (!mGame || mGame->GetPlayers().empty()) {
        return;
    }

    Player* player = mGame->GetPlayers()[0];
    if (!player) {
        return;
    }

    if (ImGui::CollapsingHeader("プレイヤー")) {
        const glm::vec3 pos = player->GetPos();

        ImGui::Text("位置");
        ImGui::Text("X: %.2f", pos.x);
        ImGui::Text("Y: %.2f", pos.y);
        ImGui::Text("Z: %.2f", pos.z);

        float moveSpeed = player->GetMoveSpeed();

        if (ImGui::SliderFloat("移動速度", &moveSpeed, 0.0f, 30.0f, "%.1f")) {
            moveSpeed = std::round(moveSpeed * 10.0f) / 10.0f;
            player->SetMoveSpeed(moveSpeed);
        }

        int hp = player->GetHp();
        if (ImGui::SliderInt("体力", &hp, 1, 100)) {
            player->SetHp(hp);
        }

        if (ImGui::Button("保存する")) {
            const std::string filePath = "../assets/data/actor/players.yaml";

            YAML::Node config;

            try {
                config = YAML::LoadFile(filePath);
            } catch (const YAML::Exception& e) {
                std::cerr << "Failed to load yaml: " << filePath << std::endl;
                std::cerr << e.what() << std::endl;
                return;
            }

            const std::string sequenceName = "players";

            SetYamlSequenceValue(config, sequenceName, 0, "moveSpeed", player->GetMoveSpeed());
            SetYamlSequenceValue(config, sequenceName, 0, "hp", player->GetHp());

            SaveYamlFile(filePath, config);
        }
    }
}

void DebugUIRenderer::DrawCamera()
{
    if (!mGame || !mGame->GetCameraSystem()) {
        return;
    }

    CameraSystem* cameraSystem = mGame->GetCameraSystem();

    if (ImGui::CollapsingHeader("カメラ")) {
        const glm::vec3 cameraPos = cameraSystem->GetCameraPos();

        ImGui::Text("位置");
        ImGui::Text("X: %.2f", cameraPos.x);
        ImGui::Text("Y: %.2f", cameraPos.y);
        ImGui::Text("Z: %.2f", cameraPos.z);
    }
}

// void DebugUIRenderer::DrawStage1()
// {
//     std::vector<Crystal*> crystals = mGame->GetCurrentStage()->GetPlanets()[0]->GetCrystals();
//     if (ImGui::CollapsingHeader("Stage1")) {
//         if (ImGui::CollapsingHeader("Crystal")) {
//             float height = crystals[1]->
//         }
//     }
// }

// void DebugUIRenderer::DrawDebugDrawSettings()
// {
//     if (ImGui::CollapsingHeader("Debug Draw")) {
//         // 後で実装する用
//         ImGui::Text("Debug draw settings will be here.");
//     }
// }

bool DebugUIRenderer::SaveYamlFile(const std::string& filePath, const YAML::Node& config)
{
    std::ofstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open yaml for writing: " << filePath << std::endl;
        return false;
    }

    file << config;
    return true;
}