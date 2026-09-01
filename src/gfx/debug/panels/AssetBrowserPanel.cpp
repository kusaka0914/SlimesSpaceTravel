#include "gfx/debug/panels/AssetBrowserPanel.h"

#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"
#include "gfx/UIRenderer.h"
#include "gfx/video/TutorialVideoPlayer.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <vector>

namespace {
constexpr int VideoThumbnailMaximumWidth = 160;
constexpr int VideoThumbnailMaximumHeight = 90;

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

ImVec2 CalculateAspectFitSize(
    int sourceWidth,
    int sourceHeight,
    float maximumWidth,
    float maximumHeight)
{
    if (sourceWidth <= 0 || sourceHeight <= 0 ||
        maximumWidth <= 0.0f || maximumHeight <= 0.0f) {
        return ImVec2(0.0f, 0.0f);
    }

    const float widthScale =
        maximumWidth / static_cast<float>(sourceWidth);
    const float heightScale =
        maximumHeight / static_cast<float>(sourceHeight);
    const float fitScale = std::min(widthScale, heightScale);
    return ImVec2(
        static_cast<float>(sourceWidth) * fitScale,
        static_cast<float>(sourceHeight) * fitScale);
}
}

AssetBrowserPanel::AssetBrowserPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mThumbnailVideoPlayer(std::make_unique<TutorialVideoPlayer>()),
      mPreviewVideoPlayer(std::make_unique<TutorialVideoPlayer>()),
      mModelThumbnailRenderer(
          std::make_unique<EditorModelThumbnailRenderer>(context.game))
{
}

AssetBrowserPanel::~AssetBrowserPanel()
{
    ClearVideoThumbnails();
}

void AssetBrowserPanel::Draw()
{
    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }

    mContext.assetCatalog->EnsureScanned();
    UpdateVideoThumbnailGeneration();
    mModelThumbnailRenderer->BeginFrame();

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
        ClearVideoThumbnails();
        mModelThumbnailRenderer->Clear();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", mContext.assetCatalog->GetScanStatus().c_str());

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY |
        ImGuiTableFlags_Resizable;
    if (ImGui::BeginTable("AssetBrowserTable", 3, tableFlags, ImVec2(0.0f, 0.0f))) {
        ImGui::TableSetupColumn("種類", ImGuiTableColumnFlags_WidthFixed, 72.0f);
        ImGui::TableSetupColumn("プレビュー", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("アセット", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        std::vector<const EditorAssetInfo*> visibleAssets;
        visibleAssets.reserve(mContext.assetCatalog->GetAllAssets().size());
        for (const EditorAssetInfo& asset : mContext.assetCatalog->GetAllAssets()) {
            if (ShouldShow(asset)) {
                visibleAssets.push_back(&asset);
            }
        }

        constexpr float assetRowHeight = 72.0f;
        ImGuiListClipper clipper;
        clipper.Begin(
            static_cast<int>(visibleAssets.size()),
            assetRowHeight);
        while (clipper.Step()) {
            for (int assetIndex = clipper.DisplayStart;
                 assetIndex < clipper.DisplayEnd;
                 ++assetIndex) {
                const EditorAssetInfo& asset = *visibleAssets[assetIndex];
                ImGui::TableNextRow(ImGuiTableRowFlags_None, assetRowHeight);
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(ResolveTypeLabel(asset.type));

                ImGui::TableSetColumnIndex(1);
                ImGui::PushID(asset.relativePath.c_str());
                DrawAssetPreview(asset);
                ImGui::PopID();

                ImGui::TableSetColumnIndex(2);
                ImGui::PushID(asset.relativePath.c_str());
                const bool isSelected =
                    mSelectedAssetPath == asset.relativePath;
                if (asset.type == EditorAssetType::Video) {
                    if (ImGui::SmallButton("再生##videoPreview")) {
                        RequestVideoPreview(asset.relativePath);
                    }
                    ImGui::SameLine();
                }
                if (ImGui::Selectable(
                        asset.relativePath.c_str(),
                        isSelected)) {
                    mSelectedAssetPath = asset.relativePath;
                }

                DrawAssetDragSource(asset);
                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }

    DrawVideoPreviewPopup();
}

void AssetBrowserPanel::DrawAssetPreview(
    const EditorAssetInfo& asset)
{
    if (asset.type == EditorAssetType::Model) {
        const GLuint thumbnailTexture =
            mModelThumbnailRenderer->ResolveThumbnail(
                asset.relativePath);
        if (thumbnailTexture == 0) {
            ImGui::TextDisabled(
                mModelThumbnailRenderer->HasFailed(asset.relativePath)
                    ? "読込失敗"
                    : "生成中");
            return;
        }

        if (ImGui::ImageButton(
                "##modelThumbnail",
                static_cast<ImTextureID>(thumbnailTexture),
                ImVec2(64.0f, 64.0f),
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f))) {
            mSelectedAssetPath = asset.relativePath;
        }
        DrawAssetDragSource(asset);
        return;
    }

    if (asset.type == EditorAssetType::Texture) {
        if (!mContext.uiRenderer ||
            !mContext.uiRenderer->RegisterCustomUITexture(
                asset.relativePath)) {
            ImGui::TextDisabled("読込失敗");
            return;
        }

        const unsigned int textureHandle =
            mContext.uiRenderer->GetCustomUITextureHandle(
                asset.relativePath);
        if (textureHandle == 0) {
            ImGui::TextDisabled("読込失敗");
            return;
        }

        if (ImGui::ImageButton(
                "##assetThumbnail",
                static_cast<ImTextureID>(textureHandle),
                ImVec2(64.0f, 64.0f),
                ImVec2(0.0f, 1.0f),
                ImVec2(1.0f, 0.0f))) {
            mSelectedAssetPath = asset.relativePath;
        }
        DrawAssetDragSource(asset);
        return;
    }

    if (asset.type != EditorAssetType::Video) {
        ImGui::TextDisabled("-");
        return;
    }

    RequestVideoThumbnail(asset.relativePath);
    const auto thumbnailIterator =
        mVideoThumbnails.find(asset.relativePath);
    bool wasPreviewClicked = false;
    if (thumbnailIterator != mVideoThumbnails.end()) {
        const VideoThumbnail& thumbnail = thumbnailIterator->second;
        const ImVec2 thumbnailSize = CalculateAspectFitSize(
            thumbnail.width,
            thumbnail.height,
            64.0f,
            64.0f);
        wasPreviewClicked = ImGui::ImageButton(
            "##videoThumbnail",
            static_cast<ImTextureID>(thumbnail.textureHandle),
            thumbnailSize,
            ImVec2(0.0f, 0.0f),
            ImVec2(1.0f, 1.0f));
    } else {
        const bool thumbnailFailed =
            mFailedVideoThumbnails.contains(asset.relativePath);
        wasPreviewClicked = ImGui::Button(
            thumbnailFailed ? "再生" : "読込中",
            ImVec2(64.0f, 64.0f));
    }

    if (wasPreviewClicked) {
        mSelectedAssetPath = asset.relativePath;
    }
    if (ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        RequestVideoPreview(asset.relativePath);
    }
    DrawAssetDragSource(asset);
}

void AssetBrowserPanel::RequestVideoPreview(
    const std::string& assetRelativePath)
{
    mRequestedVideoPreviewPath = assetRelativePath;
}

void AssetBrowserPanel::DrawVideoPreviewPopup()
{
    constexpr const char* popupId = "動画プレビュー";
    bool wasPreviewRequestedThisFrame = false;
    if (!mRequestedVideoPreviewPath.empty()) {
        wasPreviewRequestedThisFrame = true;
        mPreviewVideoPath = mRequestedVideoPreviewPath;
        mRequestedVideoPreviewPath.clear();
        mPreviewVideoPlayer->Stop();
        mPreviewVideoPlayer->Play(
            "asset-browser-preview",
            mPreviewVideoPath,
            true);
        mIsVideoPreviewOpen = true;
        ImGui::OpenPopup(popupId);
    }

    ImGui::SetNextWindowSize(
        ImVec2(800.0f, 560.0f),
        ImGuiCond_FirstUseEver);
    bool shouldKeepPreviewOpen = mIsVideoPreviewOpen;
    const bool isPreviewPopupVisible = ImGui::BeginPopupModal(
            popupId,
            &shouldKeepPreviewOpen,
            ImGuiWindowFlags_NoCollapse);
    if (isPreviewPopupVisible) {
        ImGui::TextWrapped("%s", mPreviewVideoPath.c_str());

        mPreviewVideoPlayer->Update();
        const GLuint textureHandle =
            mPreviewVideoPlayer->GetTextureHandle();
        const int videoWidth =
            mPreviewVideoPlayer->GetVideoWidth();
        const int videoHeight =
            mPreviewVideoPlayer->GetVideoHeight();
        if (textureHandle != 0 &&
            videoWidth > 0 && videoHeight > 0) {
            const ImVec2 availableSize =
                ImGui::GetContentRegionAvail();
            const ImVec2 previewSize = CalculateAspectFitSize(
                videoWidth,
                videoHeight,
                availableSize.x,
                std::max(120.0f, availableSize.y - 48.0f));
            const float horizontalOffset =
                std::max(0.0f, (availableSize.x - previewSize.x) * 0.5f);
            ImGui::SetCursorPosX(
                ImGui::GetCursorPosX() + horizontalOffset);
            ImGui::Image(
                static_cast<ImTextureID>(textureHandle),
                previewSize,
                ImVec2(0.0f, 0.0f),
                ImVec2(1.0f, 1.0f));
        } else if (!mPreviewVideoPlayer->GetLastError().empty()) {
            ImGui::TextColored(
                ImVec4(1.0f, 0.35f, 0.25f, 1.0f),
                "%s",
                mPreviewVideoPlayer->GetLastError().c_str());
        } else {
            ImGui::TextDisabled("動画を読み込んでいます...");
        }

        if (ImGui::Button("最初から再生")) {
            mPreviewVideoPlayer->Stop();
            mPreviewVideoPlayer->Play(
                "asset-browser-preview",
                mPreviewVideoPath,
                true);
        }
        ImGui::SameLine();
        if (ImGui::Button("閉じる")) {
            shouldKeepPreviewOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if (!isPreviewPopupVisible &&
        !wasPreviewRequestedThisFrame &&
        mIsVideoPreviewOpen) {
        shouldKeepPreviewOpen = false;
    }

    if (!shouldKeepPreviewOpen && mIsVideoPreviewOpen) {
        mPreviewVideoPlayer->Stop();
        mPreviewVideoPath.clear();
    }
    mIsVideoPreviewOpen = shouldKeepPreviewOpen;
}

void AssetBrowserPanel::UpdateVideoThumbnailGeneration()
{
    if (mThumbnailVideoPath.empty()) {
        return;
    }

    mThumbnailVideoPlayer->Update();
    if (mThumbnailVideoPlayer->GetTextureHandle() != 0) {
        if (!CaptureVideoThumbnail(mThumbnailVideoPath)) {
            mFailedVideoThumbnails.insert(mThumbnailVideoPath);
        }
        mThumbnailVideoPlayer->Stop();
        mThumbnailVideoPath.clear();
        return;
    }

    if (!mThumbnailVideoPlayer->GetLastError().empty()) {
        mFailedVideoThumbnails.insert(mThumbnailVideoPath);
        mThumbnailVideoPlayer->Stop();
        mThumbnailVideoPath.clear();
    }
}

void AssetBrowserPanel::RequestVideoThumbnail(
    const std::string& assetRelativePath)
{
    const bool wasAlreadyProcessed =
        mVideoThumbnails.contains(assetRelativePath) ||
        mFailedVideoThumbnails.contains(assetRelativePath);
    if (wasAlreadyProcessed || !mThumbnailVideoPath.empty()) {
        return;
    }

    if (!mThumbnailVideoPlayer->Play(
            "asset-browser-thumbnail",
            assetRelativePath,
            false)) {
        mFailedVideoThumbnails.insert(assetRelativePath);
        return;
    }
    mThumbnailVideoPath = assetRelativePath;
}

bool AssetBrowserPanel::CaptureVideoThumbnail(
    const std::string& assetRelativePath)
{
    const GLuint sourceTextureHandle =
        mThumbnailVideoPlayer->GetTextureHandle();
    const int sourceWidth =
        mThumbnailVideoPlayer->GetVideoWidth();
    const int sourceHeight =
        mThumbnailVideoPlayer->GetVideoHeight();
    if (sourceTextureHandle == 0 ||
        sourceWidth <= 0 || sourceHeight <= 0) {
        return false;
    }

    const ImVec2 fittedSize = CalculateAspectFitSize(
        sourceWidth,
        sourceHeight,
        static_cast<float>(VideoThumbnailMaximumWidth),
        static_cast<float>(VideoThumbnailMaximumHeight));
    const int thumbnailWidth =
        std::max(1, static_cast<int>(fittedSize.x));
    const int thumbnailHeight =
        std::max(1, static_cast<int>(fittedSize.y));

    std::vector<std::uint8_t> sourcePixels(
        static_cast<std::size_t>(sourceWidth) *
        static_cast<std::size_t>(sourceHeight) * 4U);

    GLint previousTextureBinding = 0;
    GLint previousPackAlignment = 0;
    GLint previousUnpackAlignment = 0;
    glGetIntegerv(
        GL_TEXTURE_BINDING_2D,
        &previousTextureBinding);
    glGetIntegerv(
        GL_PACK_ALIGNMENT,
        &previousPackAlignment);
    glGetIntegerv(
        GL_UNPACK_ALIGNMENT,
        &previousUnpackAlignment);

    glBindTexture(GL_TEXTURE_2D, sourceTextureHandle);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTexImage(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        sourcePixels.data());

    std::vector<std::uint8_t> thumbnailPixels(
        static_cast<std::size_t>(thumbnailWidth) *
        static_cast<std::size_t>(thumbnailHeight) * 4U);
    for (int thumbnailY = 0;
         thumbnailY < thumbnailHeight;
         ++thumbnailY) {
        const int sourceY = std::min(
            sourceHeight - 1,
            thumbnailY * sourceHeight / thumbnailHeight);
        for (int thumbnailX = 0;
             thumbnailX < thumbnailWidth;
             ++thumbnailX) {
            const int sourceX = std::min(
                sourceWidth - 1,
                thumbnailX * sourceWidth / thumbnailWidth);
            const std::size_t sourceOffset =
                (static_cast<std::size_t>(sourceY) * sourceWidth +
                 sourceX) *
                4U;
            const std::size_t thumbnailOffset =
                (static_cast<std::size_t>(thumbnailY) * thumbnailWidth +
                 thumbnailX) *
                4U;
            std::copy_n(
                sourcePixels.data() + sourceOffset,
                4,
                thumbnailPixels.data() + thumbnailOffset);
        }
    }

    GLuint thumbnailTextureHandle = 0;
    glGenTextures(1, &thumbnailTextureHandle);
    glBindTexture(GL_TEXTURE_2D, thumbnailTextureHandle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        thumbnailWidth,
        thumbnailHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        thumbnailPixels.data());

    glBindTexture(
        GL_TEXTURE_2D,
        static_cast<GLuint>(previousTextureBinding));
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);

    mVideoThumbnails.emplace(
        assetRelativePath,
        VideoThumbnail{
            thumbnailTextureHandle,
            thumbnailWidth,
            thumbnailHeight});
    return true;
}

void AssetBrowserPanel::ClearVideoThumbnails()
{
    mThumbnailVideoPlayer->Stop();
    mThumbnailVideoPath.clear();
    mFailedVideoThumbnails.clear();
    for (const auto& [assetPath, thumbnail] : mVideoThumbnails) {
        (void)assetPath;
        if (thumbnail.textureHandle != 0) {
            glDeleteTextures(1, &thumbnail.textureHandle);
        }
    }
    mVideoThumbnails.clear();
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

void AssetBrowserPanel::DrawAssetDragSource(
    const EditorAssetInfo& asset) const
{
    if (!ImGui::BeginDragDropSource()) {
        return;
    }

    EditorAssetDragDrop::SetPayload(asset.type, asset.relativePath);
    ImGui::TextUnformatted(asset.relativePath.c_str());
    ImGui::EndDragDropSource();
}
