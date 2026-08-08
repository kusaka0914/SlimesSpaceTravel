#include "gfx/debug/panels/AssetBrowserPanel.h"

#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>

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

AssetBrowserPanel::AssetBrowserPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void AssetBrowserPanel::Draw()
{
    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }

    mContext.assetCatalog->EnsureScanned();

    ImGui::SetNextItemWidth(240.0f);
    ImGui::InputTextWithHint(
        "##AssetBrowserSearch",
        "ファイル名またはパスで検索",
        mSearchText.data(),
        mSearchText.size());
    ImGui::SameLine();

    constexpr const char* filterLabels[] = {
        "すべて",
        "モデル",
        "画像",
        "動画",
    };
    for (int filterIndex = 0; filterIndex < IM_ARRAYSIZE(filterLabels); ++filterIndex) {
        if (filterIndex > 0) {
            ImGui::SameLine();
        }
        const int assetType = filterIndex - 1;
        if (ImGui::Selectable(
                filterLabels[filterIndex],
                mSelectedType == assetType,
                0,
                ImVec2(64.0f, 0.0f))) {
            mSelectedType = assetType;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("再読み込み")) {
        mContext.assetCatalog->Refresh();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", mContext.assetCatalog->GetScanStatus().c_str());

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("AssetBrowserTable", 2, tableFlags, ImVec2(0.0f, 0.0f))) {
        ImGui::TableSetupColumn("種類", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupColumn("アセット", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const EditorAssetInfo& asset : mContext.assetCatalog->GetAllAssets()) {
            if (!ShouldShow(asset)) {
                continue;
            }

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(ResolveTypeLabel(asset.type));
            ImGui::TableSetColumnIndex(1);

            ImGui::PushID(static_cast<int>(asset.type));
            const bool isSelected = mSelectedAssetPath == asset.relativePath;
            if (ImGui::Selectable(
                    asset.relativePath.c_str(),
                    isSelected,
                    ImGuiSelectableFlags_SpanAllColumns)) {
                mSelectedAssetPath = asset.relativePath;
            }

            if (ImGui::BeginDragDropSource()) {
                const char* payloadType = ResolveDragDropPayload(asset.type);
                ImGui::SetDragDropPayload(
                    payloadType,
                    asset.relativePath.c_str(),
                    asset.relativePath.size() + 1);
                ImGui::TextUnformatted(asset.relativePath.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

bool AssetBrowserPanel::ShouldShow(const EditorAssetInfo& asset) const
{
    if (mSelectedType >= 0 &&
        static_cast<int>(asset.type) != mSelectedType) {
        return false;
    }

    const std::string search = ToLower(mSearchText.data());
    return search.empty() ||
           ToLower(asset.relativePath).find(search) != std::string::npos;
}

const char* AssetBrowserPanel::ResolveTypeLabel(EditorAssetType type) const
{
    switch (type) {
    case EditorAssetType::Model:
        return "モデル";
    case EditorAssetType::Texture:
        return "画像";
    case EditorAssetType::Video:
        return "動画";
    case EditorAssetType::Count:
        return "不明";
    }

    return "不明";
}

const char* AssetBrowserPanel::ResolveDragDropPayload(EditorAssetType type) const
{
    switch (type) {
    case EditorAssetType::Model:
        return "EDITOR_MODEL_ASSET";
    case EditorAssetType::Texture:
        return "EDITOR_TEXTURE_ASSET";
    case EditorAssetType::Video:
        return "EDITOR_VIDEO_ASSET";
    case EditorAssetType::Count:
        return "EDITOR_UNKNOWN_ASSET";
    }

    return "EDITOR_UNKNOWN_ASSET";
}
