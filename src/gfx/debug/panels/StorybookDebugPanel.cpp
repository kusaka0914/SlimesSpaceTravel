#include "gfx/debug/panels/StorybookDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "imgui.h"
#include "system/UILoadSystem.h"
#include "system/SceneSystem.h"

#include <array>
#include <cstddef>
#include <cstdio>

namespace {
struct TextGroup { const char* screen; const char* id; };
constexpr TextGroup OpeningGroups[] = {
    {"opening", "openingText"}, {"opening", "talkWithMotherText"},
    {"opening", "talkWithDoctorText"},
};
constexpr TextGroup EndingGroups[] = {{"ending", "endingText"}};
}

StorybookDebugPanel::StorybookDebugPanel(DebugEditorContext& context) : DebugPanel(context)
{
    Reload();
}

void StorybookDebugPanel::Reload()
{
    if (mContext.uiRenderer && mContext.uiRenderer->GetUILoadSystem()) {
        mContext.uiRenderer->GetUILoadSystem()->ReloadUIInfo("../assets/data/ui/ui.yaml");
    }
    mStatus = mConfig.Load() ? "storybook.yaml を読み込みました" : "読み込みに失敗しました";
    if (mContext.game && mContext.game->GetSceneSystem()) {
        mContext.game->GetSceneSystem()->ReloadStorybookConfig();
    }
    mConfig.RemoveTrack("openingIntro");
    mConfig.RemoveTrack("openingMother");
    mConfig.RemoveTrack("openingDoctor");
}

void StorybookDebugPanel::NormalizeTrackImageCount(const char* trackId, bool isOpening)
{
    if (!mContext.uiRenderer || !mContext.uiRenderer->GetUILoadSystem()) return;
    const TextGroup* groups = isOpening ? OpeningGroups : EndingGroups;
    const std::size_t count = isOpening ? IM_ARRAYSIZE(OpeningGroups) : IM_ARRAYSIZE(EndingGroups);
    std::size_t pages = 0;
    const auto& infos = mContext.uiRenderer->GetUILoadSystem()->GetEditableTextInfos();
    for (std::size_t i = 0; i < count; ++i) {
        const auto it = infos.find(std::string(groups[i].screen) + "." + groups[i].id);
        if (it != infos.end()) pages += it->second.texts.size();
    }
    mConfig.GetEditablePageImages(trackId).resize(pages);
}

void StorybookDebugPanel::Draw()
{
    if (!mContext.uiRenderer || !mContext.uiRenderer->GetUILoadSystem()) {
        ImGui::TextUnformatted("UI設定を利用できません。"); return;
    }
    if (ImGui::Button("保存")) {
        NormalizeTrackImageCount("opening", true);
        NormalizeTrackImageCount("ending", false);
        const bool savedText = mContext.uiRenderer->GetUILoadSystem()->SaveUIInfo("../assets/data/ui/ui.yaml");
        const bool savedImages = mConfig.Save();
        mStatus = savedText && savedImages ? "会話文と画像設定を保存しました" : "保存に失敗しました";
        if (savedImages && mContext.game &&
            mContext.game->GetSceneSystem()) {
            mContext.game->GetSceneSystem()->ReloadStorybookConfig();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込")) Reload();
    ImGui::SameLine(); ImGui::TextUnformatted(mStatus.c_str());
    ImGui::Separator();

    ImGui::BeginChild("StorybookTrackList", ImVec2(180.0f, 0.0f), true);
    if (ImGui::Selectable("オープニング", mSelectedTrack == 0)) mSelectedTrack = 0;
    if (ImGui::Selectable("エンディング", mSelectedTrack == 1)) mSelectedTrack = 1;
    ImGui::EndChild(); ImGui::SameLine();
    if (mSelectedTrack == 0) DrawTrack("opening", "オープニング", true);
    else DrawTrack("ending", "エンディング", false);
}

void StorybookDebugPanel::DrawTrack(const char* trackId, const char* displayName, bool isOpening)
{
    auto& infos = mContext.uiRenderer->GetUILoadSystem()->GetEditableTextInfos();
    const TextGroup* groups = isOpening ? OpeningGroups : EndingGroups;
    const std::size_t groupCount = isOpening ? IM_ARRAYSIZE(OpeningGroups) : IM_ARRAYSIZE(EndingGroups);
    NormalizeTrackImageCount(trackId, isOpening);
    std::vector<std::string>& images = mConfig.GetEditablePageImages(trackId);

    ImGui::BeginChild("StorybookPageEditor", ImVec2(0.0f, 0.0f), true);
    ImGui::TextUnformatted(displayName);
    ImGui::TextUnformatted("会話文と画像を各ページごとに編集できます。画像は選択またはドラッグ＆ドロップで設定します。");
    std::size_t globalIndex = 0;
    for (std::size_t groupIndex = 0; groupIndex < groupCount; ++groupIndex) {
        const TextGroup& group = groups[groupIndex];
        const auto found = infos.find(std::string(group.screen) + "." + group.id);
        if (found == infos.end()) continue;
        std::vector<std::string>& texts = found->second.texts;
        for (std::size_t localIndex = 0; localIndex < texts.size(); ++localIndex, ++globalIndex) {
            ImGui::PushID(static_cast<int>(globalIndex)); ImGui::Separator();
            ImGui::Text("ページ %d", static_cast<int>(globalIndex + 1));
            std::array<char, 4096> buffer{};
            std::snprintf(buffer.data(), buffer.size(), "%s", texts[localIndex].c_str());
            if (ImGui::InputTextMultiline("会話文", buffer.data(), buffer.size(), ImVec2(-1.0f, 80.0f))) {
                texts[localIndex] = buffer.data();
            }
            DrawImagePicker(images[globalIndex]);
            if (ImGui::Button("この次にページを追加")) {
                texts.insert(texts.begin() + static_cast<std::ptrdiff_t>(localIndex + 1), "新しい会話");
                images.insert(images.begin() + static_cast<std::ptrdiff_t>(globalIndex + 1), "");
            }
            ImGui::SameLine();
            if (ImGui::Button("このページを削除")) {
                texts.erase(texts.begin() + static_cast<std::ptrdiff_t>(localIndex));
                images.erase(images.begin() + static_cast<std::ptrdiff_t>(globalIndex));
                ImGui::PopID(); ImGui::EndChild(); return;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
}

void StorybookDebugPanel::DrawImagePicker(std::string& imagePath)
{
    if (!mContext.assetCatalog) return;
    mContext.assetCatalog->EnsureScanned();
    ImGui::TextWrapped("画像: %s", imagePath.empty() ? "未設定（従来背景）" : imagePath.c_str());
    ImGui::Button("画像アセットをここへドロップ", ImVec2(-1.0f, 0.0f));
    std::string droppedPath;
    if (EditorAssetDragDrop::AcceptPath(EditorAssetType::Texture, droppedPath)) imagePath = droppedPath;
    if (ImGui::BeginCombo("画像を選択", imagePath.empty() ? "未設定" : imagePath.c_str())) {
        if (ImGui::Selectable("未設定（従来背景）", imagePath.empty())) imagePath.clear();
        for (const std::string& asset : mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
            if (ImGui::Selectable(asset.c_str(), imagePath == asset)) imagePath = asset;
        }
        ImGui::EndCombo();
    }
    if (!imagePath.empty() && mContext.uiRenderer->RegisterCustomUITexture(imagePath)) {
        ImGui::Image(static_cast<ImTextureID>(mContext.uiRenderer->GetCustomUITextureHandle(imagePath)),
                     ImVec2(240.0f, 135.0f), ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
}
