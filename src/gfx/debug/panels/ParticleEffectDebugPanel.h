#pragma once

#include "gfx/debug/DebugPanel.h"

#include "effect/particle/ParticleTypes.h"

#include <glm/glm.hpp>

#include <array>
#include <string>

class ParticleSystem;

// ParticleSystemのエフェクト・エミッター定義を編集し、ワールド上の試し再生と保存を提供する。
class ParticleEffectDebugPanel : public DebugPanel {
public:
    explicit ParticleEffectDebugPanel(DebugEditorContext& context);

    // 定義を直接編集する。試し再生は実際の粒子を発生させ、フリーカメラ中は粒子の更新も行う。
    void Draw() override;

private:
    void DrawToolbar(ParticleSystem& particleSystem);
    void DrawEffectList(ParticleSystem& particleSystem);
    void DrawEffectEditor(ParticleSystem& particleSystem);
    void DrawPreviewControls(ParticleSystem& particleSystem);
    void DrawEmitterList(ParticleSystem& particleSystem);
    void DrawEmitterInspector(ParticleSystem& particleSystem);
    bool ApplyTextureAsset(ParticleEmitterDefinition& emitter, const std::string& assetPath);
    bool DrawTexturePicker(ParticleEmitterDefinition& emitter);

    void SelectEffect(ParticleSystem& particleSystem, const std::string& effectId);
    void SelectEmitter(ParticleSystem& particleSystem, int emitterIndex);

    ParticleSpawnContext BuildPreviewContext() const;
    void EmitPreview(ParticleSystem& particleSystem);

    void SyncTexturePathBuffer(const ParticleEmitterDefinition& emitter);
    void ResetSelectionIfInvalid(ParticleSystem& particleSystem);

    std::array<char, 128> mNewEffectIdBuffer = {"new_effect"};
    std::array<char, 256> mTexturePathBuffer = {"spark_dot.png"};
    std::array<char, 128> mTextureAssetFilter = {};

    std::string mSelectedEffectId;
    std::string mStatusMessage;

    int mSelectedEmitterIndex = -1;
    int mPreviewPositionMode = 1;

    bool mShouldPreviewSelectedEmitterOnly = false;
    bool mIsAutoPreviewEnabled = false;
    bool mUsesPlayerAxes = true;

    float mAutoPreviewInterval = 0.6f;
    float mAutoPreviewTimer = 0.0f;
    float mPreviewScale = 1.0f;
    float mPreviewForwardDistance = 2.0f;

    glm::vec3 mCustomPreviewPosition{0.0f};
    glm::vec3 mPreviewNormal{0.0f, 1.0f, 0.0f};
    glm::vec3 mPreviewDirection{0.0f, 0.0f, 1.0f};
};
