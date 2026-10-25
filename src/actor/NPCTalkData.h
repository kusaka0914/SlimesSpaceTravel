#pragma once

#include "text/RubyText.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

// 会話中のカメラが注目する、ステージシーケンス内の配置オブジェクト。
struct NPCTalkCameraFocusTarget {
    std::string sequenceName;
    int yamlIndex = -1;

    bool IsValid() const { return !sequenceName.empty() && yamlIndex >= 0; }
};

enum class NPCProximityMessageMode { Disabled = 0, AfterTalk, Always };

// ページを進めるために必要なプレイヤー操作。
enum class TalkPageAdvanceCondition { Confirm = 0, PlayerSwitch, Jump };

const char* GetTalkPageAdvanceConditionId(TalkPageAdvanceCondition condition);

// 未知のIDはConfirmとして扱う。
TalkPageAdvanceCondition ParseTalkPageAdvanceConditionId(const std::string& conditionId);

// 会話本文と各ページの設定を管理する。プレイヤー探索や進行状況の判定はNPCが担当する。
// indexは保存順のページ番号。範囲外への変更は無視し、取得時は未設定値を返す。
class NPCTalkData {
public:
    void AddTalkTexts(const std::string& text);
    void RemoveTalkText(std::size_t index);

    // 本文が変わった場合、その本文に対応していたルビを破棄する。
    void SetTalkText(std::size_t index, const std::string& text);
    const std::vector<std::string>& GetTalkTexts() const { return mTexts; }

    // 負値はクリア条件のない通常会話を表す。未設定・範囲外の取得結果は-1。
    void SetTalkStageClearCondition(std::size_t index, int stageNum);
    int GetTalkStageClearCondition(std::size_t index) const;
    void SetTalkCameraFocusTarget(std::size_t index, const std::string& sequenceName, int yamlIndex);
    void ClearTalkCameraFocusTarget(std::size_t index);

    // 未設定・範囲外はnullptr。返すポインタはページの追加・削除後に取得し直すこと。
    const NPCTalkCameraFocusTarget* GetTalkCameraFocusTarget(std::size_t index) const;
    void SetTalkAdvanceCondition(std::size_t index, TalkPageAdvanceCondition condition);

    // 未設定・範囲外ではConfirmを返す。
    TalkPageAdvanceCondition GetTalkAdvanceCondition(std::size_t index) const;
    void SetTalkStartsOpeningAfterPage(std::size_t index, bool enabled);
    bool GetTalkStartsOpeningAfterPage(std::size_t index) const;
    void SetTalkStartsEndingAfterPage(std::size_t index, bool enabled);
    bool GetTalkStartsEndingAfterPage(std::size_t index) const;

    void SetTalkRubySegments(std::size_t index, std::vector<RubyTextSegment> segments);
    void SetTalkRubyReading(std::size_t talkIndex, std::size_t segmentIndex, const std::string& reading);
    void ClearTalkRubySegments(std::size_t index);

    // 範囲外は空配列。返す参照はページの追加・削除後に取得し直すこと。
    const std::vector<RubyTextSegment>& GetTalkRubySegments(std::size_t index) const;

    // ルビの親文字を連結した結果が、現在の本文と一致する場合だけ有効とする。
    bool HasValidTalkRuby(std::size_t index) const;

    // 1行表示用に実改行と文字列の「\n」を空白へ変換する。内容が変われば対応するルビを破棄する。
    void SetTalkProximityMessageText(std::size_t index, const std::string& text);
    const std::string& GetTalkProximityMessageText(std::size_t index) const;
    void SetTalkProximityMessageRubySegments(std::size_t index, std::vector<RubyTextSegment> segments);
    void SetTalkProximityMessageRubyReading(std::size_t talkIndex, std::size_t segmentIndex,
                                            const std::string& reading);
    void ClearTalkProximityMessageRubySegments(std::size_t index);
    const std::vector<RubyTextSegment>& GetTalkProximityMessageRubySegments(std::size_t index) const;
    bool HasValidTalkProximityMessageRuby(std::size_t index) const;

private:
    // ページに付随する情報を集約し、追加・削除時の更新漏れを防ぐ。
    struct PageSettings {
        std::optional<NPCTalkCameraFocusTarget> cameraFocusTarget;
        TalkPageAdvanceCondition advanceCondition = TalkPageAdvanceCondition::Confirm;
        bool startsOpeningAfterPage = false;
        bool startsEndingAfterPage = false;
        std::vector<RubyTextSegment> rubySegments;
        int stageClearCondition = -1;
        std::string proximityMessageText;
        std::vector<RubyTextSegment> proximityMessageRubySegments;
    };

    // 本文一覧をconst std::vector<std::string>&で公開するため、本文は設定と別配列に保持する。
    // 両配列は同じ要素数・同じページ順を保ち、追加・削除は上の関数だけで行う。
    std::vector<std::string> mTexts;
    std::vector<PageSettings> mPages;
};
