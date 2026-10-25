#pragma once

#include "system/UILoadSystem.h"

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

// 要素定義のコピー。描画時の表示上書きやControllerの選択状態は含めない。
struct UICanvasEditSnapshot {
    std::vector<UILoadSystem::CustomElement> customElements;
    std::unordered_map<std::string, UILoadSystem::TextureInfo> textureInfos;
    std::unordered_map<std::string, UILoadSystem::TextInfo> textInfos;
};

// カスタムUIとコード連携UIの編集前スナップショットを保持する。履歴数の下限は1。
class UICanvasEditHistory {
public:
    explicit UICanvasEditHistory(std::size_t maximumUndoCount = 20);

    // 編集前の状態を追加する。上限を超えた場合は最古の履歴を捨てる。
    void Capture(const UILoadSystem& uiLoadSystem);

    // 候補を一時適用し、saveChanges成功時だけ履歴を消費して表示上書きを解除する。
    // 履歴なし・保存失敗時はfalseで、メモリと履歴を維持する。saveChangesは永続化だけを行うこと。
    bool RestoreLatest(UILoadSystem& uiLoadSystem, const std::function<bool()>& saveChanges);

    // 直前に記録した編集を取り消す場合などに、最新の履歴だけを捨てる。
    void DiscardLatest();

private:
    std::size_t mMaximumUndoCount;
    std::vector<UICanvasEditSnapshot> mUndoSnapshots;
};
