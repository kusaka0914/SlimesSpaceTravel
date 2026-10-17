#pragma once

#include <string>
#include <unordered_map>
#include <vector>

struct StorybookTrack {
    std::vector<std::string> pageImages;
};

// 物語トラックごとのページ画像を保持し、YAML入出力と編集を提供する。
class StorybookConfig {
public:
    static constexpr const char* DefaultPath = "../assets/data/sequences/storybook.yaml";

    // 保持トラックをクリアして読む。ファイルを開けなければtrue、その他の例外はfalse。途中の定義が残る場合がある。
    bool Load(const std::string& path = DefaultPath);
    bool Save(const std::string& path = DefaultPath) const;

    // 0始まりのページを取得する。末尾超過は最終画像、トラックなし・負の番号・画像なしは空文字列。
    std::string GetPageImage(const std::string& trackId, int pageIndex) const;

    // 編集対象を返す。存在しないトラックIDでは空のトラックを新規作成する。
    std::vector<std::string>& GetEditablePageImages(const std::string& trackId);
    void RemoveTrack(const std::string& trackId);

private:
    std::unordered_map<std::string, StorybookTrack> mTracks;
};
