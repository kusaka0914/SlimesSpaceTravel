#pragma once

#include "actor/CharacterActor.h"
#include "actor/NPCTalkData.h"

#include <cstddef>
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

class Game;
class Player;

// キャラクターの接地・向きと会話可能状態を更新し、進行状況に合う会話を選択する。
// ページの保存・編集はNPCTalkData、会話画面の進行はSceneSystemが担当する。
class NPC : public CharacterActor {
public:
    // 構築・設定・更新
    NPC(Game* game);

    // 設定ルートのnpcs配列からtypeに一致するモデルとスケールを適用する。
    void ApplyConfig(const YAML::Node& configRoot, const std::string& type);
    void UpdateActor(float deltaTime) override;

    // 保存順の会話ページと進行条件
    // 範囲外への変更は無視する。本文変更時はルビを破棄する。
    void AddTalkTexts(const std::string& talkTexts) { mTalkData.AddTalkTexts(talkTexts); }
    void SetTalkText(std::size_t index, const std::string& talkText) { mTalkData.SetTalkText(index, talkText); }
    void RemoveTalkText(std::size_t index) { mTalkData.RemoveTalkText(index); }

    // stageNum < 0は通常会話。解決時にはクリア済み条件のうち最大のステージ番号を選ぶ。
    void SetTalkStageClearCondition(std::size_t index, int stageNum)
    {
        mTalkData.SetTalkStageClearCondition(index, stageNum);
    }
    const std::vector<std::string>& GetTalkTexts() const { return mTalkData.GetTalkTexts(); }
    int GetTalkStageClearCondition(std::size_t index) const { return mTalkData.GetTalkStageClearCondition(index); }

    // ページごとの進行操作・カメラ・演出
    void SetTalkCameraFocusTarget(std::size_t index, const std::string& sequenceName, int yamlIndex)
    {
        mTalkData.SetTalkCameraFocusTarget(index, sequenceName, yamlIndex);
    }
    void ClearTalkCameraFocusTarget(std::size_t index) { mTalkData.ClearTalkCameraFocusTarget(index); }
    void SetTalkAdvanceCondition(std::size_t index, TalkPageAdvanceCondition condition)
    {
        mTalkData.SetTalkAdvanceCondition(index, condition);
    }
    void SetTalkStartsOpeningAfterPage(std::size_t index, bool enabled)
    {
        mTalkData.SetTalkStartsOpeningAfterPage(index, enabled);
    }
    void SetTalkStartsEndingAfterPage(std::size_t index, bool enabled)
    {
        mTalkData.SetTalkStartsEndingAfterPage(index, enabled);
    }

    // 未設定・範囲外ではnullptr。取得した参照は会話ページの追加・削除後に取得し直すこと。
    const NPCTalkCameraFocusTarget* GetTalkCameraFocusTarget(std::size_t index) const
    {
        return mTalkData.GetTalkCameraFocusTarget(index);
    }
    TalkPageAdvanceCondition GetTalkAdvanceCondition(std::size_t index) const
    {
        return mTalkData.GetTalkAdvanceCondition(index);
    }
    bool GetTalkStartsOpeningAfterPage(std::size_t index) const
    {
        return mTalkData.GetTalkStartsOpeningAfterPage(index);
    }
    bool GetTalkStartsEndingAfterPage(std::size_t index) const { return mTalkData.GetTalkStartsEndingAfterPage(index); }

    // 会話本文のルビ
    void SetTalkRubySegments(std::size_t index, std::vector<RubyTextSegment> segments)
    {
        mTalkData.SetTalkRubySegments(index, std::move(segments));
    }
    void SetTalkRubyReading(std::size_t talkIndex, std::size_t segmentIndex, const std::string& reading)
    {
        mTalkData.SetTalkRubyReading(talkIndex, segmentIndex, reading);
    }
    void ClearTalkRubySegments(std::size_t index) { mTalkData.ClearTalkRubySegments(index); }
    const std::vector<RubyTextSegment>& GetTalkRubySegments(std::size_t index) const
    {
        return mTalkData.GetTalkRubySegments(index);
    }
    bool HasValidTalkRuby(std::size_t index) const { return mTalkData.HasValidTalkRuby(index); }

    // 現在の進行状況に対応する会話
    // resolvedIndexは進行条件で絞り込み後、indexは保存順のページ番号。
    std::vector<std::string> GetResolvedTalkTexts() const;
    const NPCTalkCameraFocusTarget* GetResolvedTalkCameraFocusTarget(std::size_t resolvedIndex) const;
    TalkPageAdvanceCondition GetResolvedTalkAdvanceCondition(std::size_t resolvedIndex) const;
    bool GetResolvedTalkStartsOpeningAfterPage(std::size_t resolvedIndex) const;
    bool GetResolvedTalkStartsEndingAfterPage(std::size_t resolvedIndex) const;
    std::optional<std::size_t> GetResolvedTalkSourceIndex(std::size_t resolvedIndex) const;
    const std::vector<RubyTextSegment>& GetResolvedTalkRubySegments(std::size_t resolvedIndex) const;
    int ResolveTalkStageClearCondition() const;

    // 頭上メッセージ
    void SetProximityMessageMode(NPCProximityMessageMode mode) { mProximityMessageMode = mode; }

    // 頭上メッセージは1行に整形する。ルビは本文を設定した後に指定する。
    void SetTalkProximityMessageText(std::size_t index, const std::string& text);
    void SetTalkProximityMessageRubySegments(std::size_t index, std::vector<RubyTextSegment> segments);
    void SetTalkProximityMessageRubyReading(std::size_t talkIndex, std::size_t segmentIndex,
                                            const std::string& reading);
    void ClearTalkProximityMessageRubySegments(std::size_t index);
    void SetProximityMessageRange(float range);
    void SetProximityMessageHeight(float height);
    void SetProximityMessageScale(float scale);
    void MarkTalkCompletedThisVisit() { mHasTalkedThisVisit = true; }
    NPCProximityMessageMode GetProximityMessageMode() const { return mProximityMessageMode; }
    const std::string& GetTalkProximityMessageText(std::size_t index) const
    {
        return mTalkData.GetTalkProximityMessageText(index);
    }
    const std::string& GetResolvedProximityMessageText() const;
    const std::vector<RubyTextSegment>& GetResolvedProximityMessageRubySegments() const;
    const std::vector<RubyTextSegment>& GetTalkProximityMessageRubySegments(std::size_t index) const
    {
        return mTalkData.GetTalkProximityMessageRubySegments(index);
    }
    bool HasValidTalkProximityMessageRuby(std::size_t index) const
    {
        return mTalkData.HasValidTalkProximityMessageRuby(index);
    }
    float GetProximityMessageRange() const { return mProximityMessageRange; }
    float GetProximityMessageHeight() const { return mProximityMessageHeight; }
    float GetProximityMessageScale() const { return mProximityMessageScale; }
    bool GetHasTalkedThisVisit() const { return mHasTalkedThisVisit; }
    bool ShouldShowProximityMessage() const;

    // 会話可能状態と会話中の振る舞い
    void SetName(const std::string& name) { mName = name; }
    void SetForcesTalkOnArrival(bool forcesTalkOnArrival) { mForcesTalkOnArrival = forcesTalkOnArrival; }
    bool GetIsTalkable() const { return mIsTalkable; }
    const std::string& GetName() const { return mName; }
    bool GetForcesTalkOnArrival() const { return mForcesTalkOnArrival; }

    // Alwaysモードは通常会話を開始しない。AfterTalkモードでは初回会話の後に頭上表示へ切り替える。
    bool CanStartRegularTalk() const;
    virtual bool ShouldUseTalkCamera() const { return true; }
    virtual bool ShouldFacePlayerDuringTalk() const { return true; }

private:
    std::vector<std::size_t> ResolveTalkIndices() const;
    std::optional<std::size_t> ResolveProximityMessageTalkIndex() const;
    void LookNearestPlayer(float deltaTime);
    void CheckTalkable();

    bool IsPlayerInTalkableRange(Player* player) const;

private:
    bool mIsTalkable;
    std::string mName;
    NPCTalkData mTalkData;
    NPCProximityMessageMode mProximityMessageMode = NPCProximityMessageMode::Disabled;
    float mProximityMessageRange = 3.0f;
    float mProximityMessageHeight = 1.8f;
    float mProximityMessageScale = 1.0f;
    bool mHasTalkedThisVisit = false;
    bool mForcesTalkOnArrival = false;
};
