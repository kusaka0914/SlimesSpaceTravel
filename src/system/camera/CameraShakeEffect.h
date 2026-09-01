#pragma once

#include <glm/glm.hpp>

enum class CameraShakePattern {
    AirStrongAttackHit,
    PlayerDamaged,
};

class CameraShakeEffect {
public:
    bool TryStart(CameraShakePattern pattern);
    void Update(float deltaTime);
    void Reset();

    bool IsActive() const { return mIsActive; }
    const glm::vec2& GetLocalOffset() const { return mLocalOffset; }

private:
    struct Profile {
        float intensity = 0.0f;
        float durationSeconds = 0.0f;
        float decayExponent = 1.0f;
        glm::vec2 directionWeights{1.0f};
    };

    static Profile ResolveProfile(CameraShakePattern pattern);
    void CalculateLocalOffset();

    CameraShakePattern mPattern = CameraShakePattern::PlayerDamaged;
    Profile mProfile;
    glm::vec2 mLocalOffset{0.0f};
    float mElapsedSeconds = 0.0f;
    unsigned int mActivationSequence = 0;
    bool mIsActive = false;
    bool mShouldPreserveInitialFrame = false;
};
