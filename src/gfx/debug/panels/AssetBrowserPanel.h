#pragma once

#include "gfx/debug/DebugPanel.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"

#include <GL/glew.h>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

class TutorialVideoPlayer;
class EditorModelThumbnailRenderer;

class AssetBrowserPanel final : public DebugPanel {
public:
    explicit AssetBrowserPanel(DebugEditorContext& context);
    ~AssetBrowserPanel() override;

    void Draw() override;

private:
    struct VideoThumbnail {
        GLuint textureHandle = 0;
        int width = 0;
        int height = 0;
    };

    bool ShouldShow(const EditorAssetInfo& asset) const;
    const char* ResolveTypeLabel(EditorAssetType type) const;
    void DrawAssetPreview(const EditorAssetInfo& asset);
    void DrawAssetDragSource(const EditorAssetInfo& asset) const;
    void RequestVideoPreview(const std::string& assetRelativePath);
    void DrawVideoPreviewPopup();
    void UpdateVideoThumbnailGeneration();
    void RequestVideoThumbnail(const std::string& assetRelativePath);
    bool CaptureVideoThumbnail(const std::string& assetRelativePath);
    void ClearVideoThumbnails();

    std::array<char, 128> mSearchText = {};
    std::string mSelectedAssetPath;
    std::string mRequestedVideoPreviewPath;
    std::string mPreviewVideoPath;
    std::string mThumbnailVideoPath;
    std::unordered_map<std::string, VideoThumbnail> mVideoThumbnails;
    std::unordered_set<std::string> mFailedVideoThumbnails;
    std::unique_ptr<TutorialVideoPlayer> mThumbnailVideoPlayer;
    std::unique_ptr<TutorialVideoPlayer> mPreviewVideoPlayer;
    std::unique_ptr<EditorModelThumbnailRenderer> mModelThumbnailRenderer;
    bool mIsVideoPreviewOpen = false;
    int mSelectedType = -1;
};
