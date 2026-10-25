#pragma once

#include "system/sequence/SequenceTypes.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Actor移動・表示・操作制御・カメラの演出定義を保持し、YAML入出力と編集を提供する。
class SequenceLibrary {
public:
    explicit SequenceLibrary(std::string filePath);

    // 全定義を読めた場合だけ置き換える。失敗時はfalseで現在の定義・借用参照を維持する。
    bool Load();
    bool Save() const;

    // 保持定義への借用ポインターを返す。未登録ならnullptr。削除・再読込で無効になる。
    const GameplaySequence* Find(std::string_view id) const;

    // 編集用の借用ポインターを返す。未登録ならnullptr。削除・再読込で無効になる。
    GameplaySequence* FindMutable(std::string_view id);

    // 空でない未登録IDをメモリ上に追加する。追加できなければfalse。保存は別途行う。
    bool Create(std::string id);

    // 重複しないIDで複製し、その借用ポインターを返す。元のIDがなければnullptr。保存は別途行う。
    GameplaySequence* Duplicate(std::string_view id);

    // 既存定義のIDを変更する。空ID・重複ID・元定義なしはfalse。外部からのID参照は更新しない。
    bool Rename(std::string_view currentId, std::string newId);
    bool Remove(std::string_view id);
    std::vector<std::string> GetIds() const;

    const std::string& GetFilePath() const { return mFilePath; }

private:
    std::string mFilePath;
    std::unordered_map<std::string, GameplaySequence> mSequences;
};
