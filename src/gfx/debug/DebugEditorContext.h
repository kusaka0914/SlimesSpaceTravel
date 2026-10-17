#pragma once

class Game;
class UIRenderer;
class EditorAssetCatalog;

// ゲーム画像の表示矩形はImGuiのスクリーン座標。sourceWidth/Heightは元テクスチャのピクセル寸法。
struct DebugEditorGameViewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    int sourceWidth = 0;
    int sourceHeight = 0;

    bool IsValid() const { return width > 0.0f && height > 0.0f && sourceWidth > 0 && sourceHeight > 0; }
};

// ImGui座標で保持する編集パネルの寸法。0以下ならLayoutControllerが初期値を選ぶ。
struct DebugEditorLayoutState {
    float rightPanelWidth = 0.0f;
    float assetBrowserHeight = 0.0f;
};

enum class PlanetMoveMode {
    WithBoundActors,
    PlanetOnly,
};

// 各編集機能が共有する参照と画面状態。ポインタはすべて非所有で、未設定時はnullptr。
struct DebugEditorContext {
    Game* game = nullptr;
    UIRenderer* uiRenderer = nullptr;
    EditorAssetCatalog* assetCatalog = nullptr;
    DebugEditorGameViewport gameViewport;
    DebugEditorLayoutState layout;
    PlanetMoveMode planetMoveMode = PlanetMoveMode::WithBoundActors;
};
