#pragma once

#include "actor/NPC.h"

class TutorialTrigger final : public NPC {
public:
    explicit TutorialTrigger(Game* game);

    void UpdateActor(float deltaTime) override;

    float GetRenderOpacity() const override;
    bool ShouldUseTalkCamera() const override { return false; }
    bool ShouldFacePlayerDuringTalk() const override { return false; }

protected:
    void OnLoadedModelChanged() override;

private:
    bool IsInsideModelBounds(const glm::vec3& worldPosition) const;

private:
    glm::vec3 mLocalBoundsMin{-0.5f};
    glm::vec3 mLocalBoundsMax{0.5f};
    bool mHasTriggeredThisVisit = false;
};
