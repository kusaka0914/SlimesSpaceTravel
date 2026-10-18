#pragma once

#include "editor/core/DebugEditorContext.h"

// UGC用の編集可能なUI要素を補完し、UIエディタへ登録する。contextの参照先は所有しない。
class UGCEditorElementRegistrar {
public:
    explicit UGCEditorElementRegistrar(DebugEditorContext& context);

    // 不足する操作ボタンのみ追加し、既存レイアウトを維持して編集対象を登録する。UI未初期化時は何もしない。
    void RegisterElements();

private:
    DebugEditorContext& mContext;
};
