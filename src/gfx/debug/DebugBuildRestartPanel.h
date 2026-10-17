#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <string>

// ビルド・再起動要求のボタンと処理状況を表示する。共有contextは非所有参照。
class DebugBuildRestartPanel {
public:
    explicit DebugBuildRestartPanel(DebugEditorContext& context);

    // 要求ボタンと状態を描く。レビュー用ビルドでは表示しない。
    void Draw();

    // 次回以降のDrawで表示する状況を置き換える。空文字列なら状況表示を消す。
    void SetStatus(const std::string& message, bool isError);

private:
    DebugEditorContext& mContext;
    std::string mStatusMessage;
    bool mIsStatusError = false;
};
