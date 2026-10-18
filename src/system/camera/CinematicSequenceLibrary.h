#pragma once

#include "system/camera/CinematicCameraTypes.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// カメラ演出定義をID別に保持し、YAML入出力とメモリ上の編集を提供する。
class CinematicSequenceLibrary {
public:
    explicit CinematicSequenceLibrary(std::string filePath);

    // 全定義を読めた場合だけ置き換える。失敗時はfalseで現在の定義・借用参照を維持する。
    bool Load();
    bool Save() const;

    // 保持定義への借用ポインターを返す。未登録ならnullptr。削除・再読込で無効になる。
    const CinematicSequence* Find(std::string_view sequenceId) const;

    // 編集用の借用ポインターを返す。未登録ならnullptr。削除・再読込で無効になる。
    CinematicSequence* FindMutable(std::string_view sequenceId);

    // 空でない未登録IDをメモリ上に追加する。追加できなければfalse。保存は別途行う。
    bool Create(std::string sequenceId);

    // 重複しないIDで複製し、その借用ポインターを返す。元のIDがなければnullptr。保存は別途行う。
    CinematicSequence* Duplicate(std::string_view sequenceId);

    // 既存定義のIDを変更する。空ID・重複ID・元定義なしはfalse。外部からのID参照は更新しない。
    bool Rename(std::string_view currentSequenceId, std::string newSequenceId);
    bool Remove(std::string_view sequenceId);

    std::vector<std::string> GetSequenceIds() const;

    const std::string& GetFilePath() const { return mFilePath; }

private:
    std::string mFilePath;
    std::unordered_map<std::string, CinematicSequence> mSequences;
};
