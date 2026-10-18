#pragma once

#include <GL/glew.h>

#include <memory>
#include <string>

class DebugUIRenderer;
class Game;
class UIRenderer;

// UIRendererからエディターへの描画・操作要求を仲介し、DebugUIRendererを所有する。
// 渡されたGameとUIRendererは非所有参照。生成・描画・破棄は有効なGLコンテキスト上で行う。
class UIDebugEditorBridge {
public:
    UIDebugEditorBridge(Game* game, UIRenderer* uiRenderer);
    ~UIDebugEditorBridge();

    // ゲーム画面のテクスチャを借用し、通常またはUGCエディターに表示する。
    void DrawEditor(GLuint gameViewTexture, int gameViewWidth, int gameViewHeight, bool isUGCEditor);
    void DrawWorkBrowser();

    // 作品のクリア検証完了を保存し、成功時にtrueを返す。
    bool CompleteUGCVerification(const std::string& workFileName);

    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void OpenEditorMenu();
    void AdjustZoom(float distanceMultiplier);
    void ChangeLayer(int layerDelta);
    void MoveSelectionOnGrid(int gridX, int gridZ);

    void NotifyTutorialReturnedFromPlaytest();

    // ステージ内Actorの破棄前に呼び、エディターの選択参照を解除する。
    void ClearStageSelectionForStageReload();

    // 再読込成功後に、旧Runtimeに対応する保存失敗の破棄を通知する。
    void NotifyStageReloaded();

    // エディターのセッションを保存／復元する。失敗時はfalseを返し、outErrorMessageに理由を設定する。
    bool SaveSession(const std::string& filePath, std::string& outErrorMessage);
    bool RestoreSession(const std::string& filePath, std::string& outErrorMessage);

    // ビルド・再起動処理の通知をエディターへ表示する。
    void SetBuildRestartStatus(const std::string& message, bool isError);

private:
    std::unique_ptr<DebugUIRenderer> mDebugUIRenderer;
};
