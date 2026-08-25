#include "gfx/debug/stage/StageActorAssetEditor.h"

#include "gfx/Renderer3D.h"
#include "Game.h"
#include "actor/Actor.h"
#include "actor/Boat.h"
#include "actor/NPC.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "imgui.h"
#include "system/MeshLoadSystem.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace {
std::string ToLower(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return text;
}
}

StageActorAssetEditor::StageActorAssetEditor(
    DebugEditorContext& context,
    Callback rebuildPhysicsWorld)
    : mContext(context),
      mRebuildPhysicsWorld(std::move(rebuildPhysicsWorld))
{
}

void StageActorAssetEditor::RequestPhysicsWorldRebuild()
{
    if (mRebuildPhysicsWorld) {
        mRebuildPhysicsWorld();
    }
}
void StageActorAssetEditor::DrawActorModelPicker(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!actor || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", actor->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedActorModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        actor->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(actor);
        RequestPhysicsWorldRebuild();
    }

    const std::string pickerId =
        "##placedActorModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedActorModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mActorModelAssetFilter.data(),
        mActorModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mActorModelAssetFilter.data());
    const std::string listId =
        "PlacedActorModelAssetPicker##" + sequenceName + std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() && ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == actor->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            actor->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(actor);
            RequestPhysicsWorldRebuild();
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled(
        "見た目と当たり判定へ即時反映されます。変更後は「保存する」を押してください。");
    ImGui::TreePop();
}

void StageActorAssetEditor::DrawNPCModelPicker(
    NPC* npc,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!npc || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", npc->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedNPCModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        npc->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(npc);
    }

    const std::string pickerId =
        "##placedNPCModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedNPCModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mNPCModelAssetFilter.data(),
        mNPCModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mNPCModelAssetFilter.data());
    const std::string listId =
        "PlacedNPCModelAssetPicker##" + sequenceName + std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() && ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == npc->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            npc->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(npc);
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled("変更後、左側の「保存する」でステージへ保存してください。");
    ImGui::TreePop();
}

void StageActorAssetEditor::DrawBoatModelPicker(
    Boat* boat,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!boat || !mContext.game || !mContext.game->GetMeshLoadSystem()) {
        return;
    }

    ImGui::TextWrapped("モデル: %s", boat->GetModelPath().c_str());
    ImGui::Button(
        ("モデルアセットをここへドロップ##placedBoatModelDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedModelPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Model,
            droppedModelPath)) {
        boat->SetModelPath(droppedModelPath);
        mContext.game->GetMeshLoadSystem()->SetActorMesh(boat);
    }

    const std::string pickerId =
        "##placedBoatModelPicker" + sequenceName + std::to_string(listIndex);
    if (!ImGui::TreeNode(("モデルを変更" + pickerId).c_str())) {
        return;
    }

    const std::string filterId =
        "##placedBoatModelFilter" + sequenceName + std::to_string(listIndex);
    ImGui::InputTextWithHint(
        filterId.c_str(),
        "モデル名で検索",
        mBoatModelAssetFilter.data(),
        mBoatModelAssetFilter.size());

    const std::vector<std::string>& modelAssets =
        mContext.assetCatalog->GetPaths(EditorAssetType::Model);
    const std::string filter = ToLower(mBoatModelAssetFilter.data());
    const std::string listId =
        "PlacedBoatModelAssetPicker##" + sequenceName +
        std::to_string(listIndex);

    ImGui::BeginChild(listId.c_str(), ImVec2(0.0f, 180.0f), true);
    for (const std::string& modelPath : modelAssets) {
        if (!filter.empty() &&
            ToLower(modelPath).find(filter) == std::string::npos) {
            continue;
        }

        const bool selected = modelPath == boat->GetModelPath();
        if (ImGui::Selectable(modelPath.c_str(), selected)) {
            boat->SetModelPath(modelPath);
            mContext.game->GetMeshLoadSystem()->SetActorMesh(boat);
        }
    }
    ImGui::EndChild();
    ImGui::TextDisabled(
        "変更後、左側の「保存する」でステージへ保存してください。");
    ImGui::TreePop();
}

void StageActorAssetEditor::DrawTextureOverrideEditor(
    Actor* actor,
    const std::string& sequenceName,
    std::size_t listIndex)
{
    if (!actor) {
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    ImGui::SeparatorText("テクスチャ");
    const std::string& selectedTexture = actor->GetTextureOverridePath();
    ImGui::TextWrapped(
        "選択中: %s",
        selectedTexture.empty() ? "モデル標準" : selectedTexture.c_str());
    ImGui::Button(
        ("画像アセットをここへドロップ##actorTextureDrop" +
         sequenceName + std::to_string(listIndex))
            .c_str(),
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        actor->SetTextureOverridePath(droppedTexturePath);
        if (mContext.game && mContext.game->GetRenderer3D() &&
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(
                droppedTexturePath) == 0) {
            mTextureAssetStatus =
                "テクスチャの読み込みに失敗しました: " +
                droppedTexturePath;
        } else {
            mTextureAssetStatus.clear();
        }
    }

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
            mContext.assetCatalog->Refresh();
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
        for (const std::string& asset :
             mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
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

void StageActorAssetEditor::DrawBulkTextureOverrideEditor(
    const std::vector<StageActorInstance>& selectedActors)
{
    if (selectedActors.empty()) {
        ImGui::TextDisabled(
            "選択中のアクターを現在のステージで取得できません。");
        return;
    }

    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません。");
        return;
    }
    mContext.assetCatalog->EnsureScanned();

    const Actor* firstActor = selectedActors.front().actor;
    if (!firstActor) {
        return;
    }

    const std::string commonTexturePath =
        firstActor->GetTextureOverridePath();
    const bool hasMixedTexturePaths = std::any_of(
        selectedActors.begin() + 1,
        selectedActors.end(),
        [&commonTexturePath](const StageActorInstance& instance) {
            return instance.actor &&
                   instance.actor->GetTextureOverridePath() !=
                       commonTexturePath;
        });

    const char* currentTextureLabel = commonTexturePath.c_str();
    if (hasMixedTexturePaths) {
        currentTextureLabel = "複数のテクスチャ";
    } else if (commonTexturePath.empty()) {
        currentTextureLabel = "モデル標準";
    }

    const auto applyTexturePath =
        [this, &selectedActors](const std::string& texturePath) {
            if (!texturePath.empty() &&
                mContext.game &&
                mContext.game->GetRenderer3D() &&
                mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(
                    texturePath) == 0) {
                mTextureAssetStatus =
                    "テクスチャの読み込みに失敗しました: " +
                    texturePath;
                return;
            }

            for (const StageActorInstance& instance : selectedActors) {
                if (instance.actor) {
                    instance.actor->SetTextureOverridePath(texturePath);
                }
            }
            mTextureAssetStatus.clear();
        };

    ImGui::SeparatorText("テクスチャ一括変更");
    ImGui::TextWrapped(
        "現在: %s",
        currentTextureLabel);

    ImGui::Button(
        "画像アセットをここへドロップ##bulkActorTextureDrop",
        ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        applyTexturePath(droppedTexturePath);
    }

    if (ImGui::TreeNode(
            "テクスチャを選ぶ##bulkActorTexturePicker")) {
        ImGui::InputTextWithHint(
            "##bulkActorTextureFilter",
            "ファイル名で検索",
            mTextureAssetFilter.data(),
            mTextureAssetFilter.size());
        ImGui::SameLine();
        if (ImGui::Button("更新##bulkActorTextureRefresh")) {
            mContext.assetCatalog->Refresh();
        }

        if (ImGui::Selectable(
                "モデル標準に戻す##bulkActorTextureDefault",
                !hasMixedTexturePaths && commonTexturePath.empty())) {
            applyTexturePath("");
        }

        ImGui::BeginChild(
            "BulkActorTextureAssetPicker",
            ImVec2(0.0f, 180.0f),
            true);
        const std::string filter =
            ToLower(mTextureAssetFilter.data());
        for (const std::string& texturePath :
             mContext.assetCatalog->GetPaths(
                 EditorAssetType::Texture)) {
            if (!filter.empty() &&
                ToLower(texturePath).find(filter) ==
                    std::string::npos) {
                continue;
            }

            const bool isSelected =
                !hasMixedTexturePaths &&
                commonTexturePath == texturePath;
            if (ImGui::Selectable(
                    texturePath.c_str(),
                    isSelected)) {
                applyTexturePath(texturePath);
            }
        }
        ImGui::EndChild();

        ImGui::TreePop();
    }

    if (!mTextureAssetStatus.empty()) {
        ImGui::TextColored(
            ImVec4(1.0f, 0.35f, 0.25f, 1.0f),
            "%s",
            mTextureAssetStatus.c_str());
    }

    if (!hasMixedTexturePaths &&
        !commonTexturePath.empty() &&
        mContext.game &&
        mContext.game->GetRenderer3D()) {
        const GLuint texture =
            mContext.game->GetRenderer3D()->GetOrLoadTextureOverride(
                commonTexturePath);
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
