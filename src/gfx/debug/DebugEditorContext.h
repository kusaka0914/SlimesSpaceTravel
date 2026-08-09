#pragma once

class Game;
class UIRenderer;
class EditorAssetCatalog;

struct DebugEditorGameViewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int sourceWidth = 0;
    int sourceHeight = 0;

    bool IsValid() const
    {
        return width > 0.0f && height > 0.0f &&
               sourceWidth > 0 && sourceHeight > 0;
    }
};

struct DebugEditorLayoutState {
    float rightPanelWidth = 0.0f;
    float assetBrowserHeight = 0.0f;
};

struct DebugEditorContext {
    Game* game = nullptr;
    UIRenderer* uiRenderer = nullptr;
    EditorAssetCatalog* assetCatalog = nullptr;
    DebugEditorGameViewport gameViewport;
    DebugEditorLayoutState layout;
};
