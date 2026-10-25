#pragma once

#include "actor/NPC.h"

#include <string>

// モデルの範囲への進入を検出してチュートリアルを開始する、通常プレイでは不可視のNPC。
// NPCの会話データを利用するが、通常NPCの向き合わせ・会話カメラ・接地更新は行わない。
class TutorialTrigger final : public NPC {
public:
    explicit TutorialTrigger(Game* game);

    // 起動成功後は再起動しない。分裂が必要な場合、範囲内で分裂した直後は退出・再進入を要求する。
    void UpdateActor(float deltaTime) override;

    float GetRenderOpacity() const override;
    bool ShouldUseTalkCamera() const override { return false; }
    bool ShouldFacePlayerDuringTalk() const override { return false; }

    // IDありなら定義済みチュートリアルを、空ならNPCに保存された会話を開始する。
    void SetTutorialId(const std::string& tutorialId) { mTutorialId = tutorialId; }
    const std::string& GetTutorialId() const { return mTutorialId; }
    void SetRequiredCompletedTutorialId(const std::string& tutorialId) { mRequiredCompletedTutorialId = tutorialId; }
    const std::string& GetRequiredCompletedTutorialId() const { return mRequiredCompletedTutorialId; }

protected:
    // モデルのローカル境界を更新する。取得できない場合は単位立方体を使う。
    void OnLoadedModelChanged() override;

private:
    bool IsInsideModelBounds(const glm::vec3& worldPosition) const;
    bool RequiresSplitPlayerToStart() const;

private:
    glm::vec3 mLocalBoundsMin{-0.5f};
    glm::vec3 mLocalBoundsMax{0.5f};
    std::string mTutorialId;
    std::string mRequiredCompletedTutorialId;
    bool mHasTriggeredThisVisit = false;
    bool mHasObservedPlayerSplitState = false;
    bool mWasPlayerSplit = false;
    bool mRequiresExitAfterSplittingInside = false;
};
