#pragma once

#include <glm/glm.hpp>

enum class CameraShakePattern {
    AirStrongAttackHit,
    PlayerDamaged,
};

// 命中・被ダメージ時のカメラ振動を、カメラ平面上のオフセットとして生成する。
class CameraShakeEffect {
public:
    // 再生中より弱い要求はfalse。それ以外は置き換え、初期オフセットを直ちに生成する。
    bool TryStart(CameraShakePattern pattern);

    // 秒単位で振動を進める。開始直後の最初のUpdateでは、命中フレームのオフセットを維持する。
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
