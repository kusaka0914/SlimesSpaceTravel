#pragma once

#include "actor/NPC.h"

#include <string>

class TutorialTrigger final : public NPC {
public:
    explicit TutorialTrigger(Game* game);

    void UpdateActor(float deltaTime) override;

    float GetRenderOpacity() const override;
    bool ShouldUseTalkCamera() const override { return false; }
    bool ShouldFacePlayerDuringTalk() const override { return false; }

    void SetTutorialId(const std::string& tutorialId)
    {
        mTutorialId = tutorialId;
    }
    const std::string& GetTutorialId() const
    {
        return mTutorialId;
    }

protected:
    void OnLoadedModelChanged() override;

private:
    bool IsInsideModelBounds(const glm::vec3& worldPosition) const;
    bool RequiresSplitPlayerToStart() const;

private:
    glm::vec3 mLocalBoundsMin{-0.5f};
    glm::vec3 mLocalBoundsMax{0.5f};
    std::string mTutorialId;
    bool mHasTriggeredThisVisit = false;
};
