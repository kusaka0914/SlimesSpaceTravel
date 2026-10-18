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

    // 成功時にトラックを置き換える。ファイルを開けなければ空にしてtrue、解析失敗時は元の定義を維持してfalse。
    bool Load(const std::string& path = DefaultPath);

    // 親ディレクトリを作成して保存する。書込・置換に失敗した場合は既存ファイルを保ってfalse。
    bool Save(const std::string& path = DefaultPath) const;

    // 0始まりのページを取得する。末尾超過は最終画像、トラックなし・負の番号・画像なしは空文字列。
    std::string GetPageImage(const std::string& trackId, int pageIndex) const;

    // 編集対象を返す。存在しないトラックIDでは空のトラックを新規作成する。
    std::vector<std::string>& GetEditablePageImages(const std::string& trackId);
    void RemoveTrack(const std::string& trackId);

private:
    std::unordered_map<std::string, StorybookTrack> mTracks;
};
