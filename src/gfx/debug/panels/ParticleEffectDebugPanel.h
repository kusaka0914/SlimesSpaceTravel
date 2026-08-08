#pragma once

#include "gfx/debug/DebugPanel.h"

#include "effect/particle/ParticleTypes.h"

#include <glm/glm.hpp>

#include <array>
#include <string>

class ParticleSystem;

class ParticleEffectDebugPanel : public DebugPanel {
public:
    explicit ParticleEffectDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawToolbar(ParticleSystem& particleSystem);
    void DrawEffectList(ParticleSystem& particleSystem);
    void DrawEffectEditor(ParticleSystem& particleSystem);
    void DrawPreviewControls(ParticleSystem& particleSystem);
    void DrawEmitterList(ParticleSystem& particleSystem);
    void DrawEmitterInspector(ParticleSystem& particleSystem);
    bool DrawTexturePicker(ParticleEmitterDefinition& emitter);

    void SelectEffect(ParticleSystem& particleSystem, const std::string& effectId);
    void SelectEmitter(ParticleSystem& particleSystem, int emitterIndex);

    ParticleSpawnContext BuildPreviewContext() const;
    void EmitPreview(ParticleSystem& particleSystem);

    void SyncTexturePathBuffer(const ParticleEmitterDefinition& emitter);
    void ResetSelectionIfInvalid(ParticleSystem& particleSystem);

private:
    char mNewEffectIdBuffer[128] = "new_effect";
    char mTexturePathBuffer[256] = "spark_dot.png";
    std::array<char, 128> mTextureAssetFilter = {};

    std::string mSelectedEffectId;
    std::string mStatusMessage;

    int mSelectedEmitterIndex = -1;
    int mPreviewPositionMode = 1;

    bool mPreviewSelectedEmitterOnly = false;
    bool mAutoPreview = false;
    bool mUsePlayerAxes = true;

    float mAutoPreviewInterval = 0.6f;
    float mAutoPreviewTimer = 0.0f;
    float mPreviewScale = 1.0f;
    float mPreviewForwardDistance = 2.0f;

    glm::vec3 mCustomPreviewPosition{0.0f};
    glm::vec3 mPreviewNormal{0.0f, 1.0f, 0.0f};
    glm::vec3 mPreviewDirection{0.0f, 0.0f, 1.0f};
};
