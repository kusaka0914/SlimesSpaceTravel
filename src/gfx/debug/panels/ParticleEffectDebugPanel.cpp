#include "gfx/debug/panels/ParticleEffectDebugPanel.h"

#include "Game.h"
#include "actor/Player.h"
#include "imgui.h"
#include "system/ParticleSystem.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {
constexpr const char* blendModeLabels[] = {
    "Alpha",
    "Additive",
};

constexpr const char* renderModeLabels[] = {
    "Billboard",
    "Velocity Aligned",
};

constexpr const char* directionModeLabels[] = {
    "Fixed",
    "Sphere",
    "Hemisphere",
    "Cone",
};

constexpr const char* previewPositionLabels[] = {
    "プレイヤー位置",
    "プレイヤー正面",
    "任意座標",
};

int ToIndex(ParticleBlendMode mode)
{
    return mode == ParticleBlendMode::Alpha ? 0 : 1;
}

ParticleBlendMode ToBlendMode(int index)
{
    return index == 0 ? ParticleBlendMode::Alpha : ParticleBlendMode::Additive;
}

int ToIndex(ParticleRenderMode mode)
{
    return mode == ParticleRenderMode::VelocityAligned ? 1 : 0;
}

ParticleRenderMode ToRenderMode(int index)
{
    return index == 1 ? ParticleRenderMode::VelocityAligned : ParticleRenderMode::Billboard;
}

int ToIndex(ParticleDirectionMode mode)
{
    switch (mode) {
    case ParticleDirectionMode::Fixed:
        return 0;
    case ParticleDirectionMode::Sphere:
        return 1;
    case ParticleDirectionMode::Hemisphere:
        return 2;
    case ParticleDirectionMode::Cone:
        return 3;
    }

    return 1;
}

ParticleDirectionMode ToDirectionMode(int index)
{
    switch (index) {
    case 0:
        return ParticleDirectionMode::Fixed;
    case 2:
        return ParticleDirectionMode::Hemisphere;
    case 3:
        return ParticleDirectionMode::Cone;
    case 1:
    default:
        return ParticleDirectionMode::Sphere;
    }
}

bool DrawRange(
    const char* label,
    ParticleFloatRange& range,
    float speed,
    float minimum,
    float maximum,
    const char* format)
{
    float values[2] = {range.min, range.max};

    if (!ImGui::DragFloat2(label, values, speed, minimum, maximum, format)) {
        return false;
    }

    range.min = std::min(values[0], values[1]);
    range.max = std::max(values[0], values[1]);
    return true;
}

ParticleEmitterDefinition CreateDefaultEmitter()
{
    ParticleEmitterDefinition emitter;
    emitter.texturePath = "spark_dot.png";
    emitter.blendMode = ParticleBlendMode::Additive;
    emitter.renderMode = ParticleRenderMode::Billboard;
    emitter.directionMode = ParticleDirectionMode::Sphere;
    emitter.count = 8;
    emitter.lifetime = {0.25f, 0.50f};
    emitter.speed = {0.5f, 2.0f};
    emitter.startSize = {0.15f, 0.30f};
    emitter.endSizeMultiplier = 0.1f;
    emitter.rotationDegrees = {0.0f, 360.0f};
    emitter.angularVelocityDegrees = {-90.0f, 90.0f};
    emitter.startColor = glm::vec4(1.0f);
    emitter.endColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
    return emitter;
}
} // namespace

ParticleEffectDebugPanel::ParticleEffectDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void ParticleEffectDebugPanel::Draw()
{
    if (!mContext.game || !mContext.game->GetParticleSystem()) {
        ImGui::TextUnformatted("ParticleSystemがありません");
        return;
    }

    ParticleSystem& particleSystem = *mContext.game->GetParticleSystem();
    ResetSelectionIfInvalid(particleSystem);

    DrawEffectControls(particleSystem);
    ImGui::Separator();
    DrawPreviewControls(particleSystem);
    ImGui::Separator();
    DrawEmitterControls(particleSystem);

    const float deltaTime = std::max(0.0f, ImGui::GetIO().DeltaTime);

    if (mAutoPreview && !mSelectedEffectId.empty()) {
        mAutoPreviewTimer -= deltaTime;
        if (mAutoPreviewTimer <= 0.0f) {
            EmitPreview(particleSystem);
            mAutoPreviewTimer = std::max(0.05f, mAutoPreviewInterval);
        }
    }

    // Game::UpdateGame()はフリーカメラ中にワールド更新を抜けるため、
    // エディタを開いている間だけここで粒子を進める。
    if (mContext.game->GetIsFreeCameraMode()) {
        particleSystem.Update(deltaTime);
    }

    if (!mStatusMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", mStatusMessage.c_str());
    }
}

void ParticleEffectDebugPanel::DrawEffectControls(ParticleSystem& particleSystem)
{
    ImGui::TextUnformatted("エフェクト");

    const std::vector<std::string> effectIds = particleSystem.GetEffectIds();
    const char* currentEffectLabel =
        mSelectedEffectId.empty() ? "未選択" : mSelectedEffectId.c_str();

    if (ImGui::BeginCombo("エフェクトID", currentEffectLabel)) {
        for (const std::string& effectId : effectIds) {
            const bool isSelected = effectId == mSelectedEffectId;

            if (ImGui::Selectable(effectId.c_str(), isSelected)) {
                SelectEffect(particleSystem, effectId);
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("新しいID", mNewEffectIdBuffer, sizeof(mNewEffectIdBuffer));

    if (ImGui::Button("新規作成")) {
        const std::string newEffectId(mNewEffectIdBuffer);

        if (particleSystem.CreateEffect(newEffectId)) {
            SelectEffect(particleSystem, newEffectId);
            mStatusMessage = "エフェクトを作成しました";
        } else {
            mStatusMessage = "IDが空、または同じIDが存在します";
        }
    }

    ImGui::SameLine();

    const ParticleEffectDefinition* selectedDefinition =
        mSelectedEffectId.empty() ? nullptr : particleSystem.FindEffect(mSelectedEffectId);

    if (!selectedDefinition) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("選択中を複製") && selectedDefinition) {
        const std::string newEffectId(mNewEffectIdBuffer);

        if (particleSystem.CreateEffect(newEffectId, *selectedDefinition)) {
            SelectEffect(particleSystem, newEffectId);
            mStatusMessage = "エフェクトを複製しました";
        } else {
            mStatusMessage = "複製先IDが空、または同じIDが存在します";
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("選択中を削除") && selectedDefinition) {
        if (particleSystem.RemoveEffect(mSelectedEffectId)) {
            mSelectedEffectId.clear();
            mSelectedEmitterIndex = -1;
            mStatusMessage = "エフェクトを削除しました";
        }
    }

    if (!selectedDefinition) {
        ImGui::EndDisabled();
    }

    if (ImGui::Button("YAML保存")) {
        mStatusMessage =
            particleSystem.SaveDefinitions()
                ? "particles.yamlへ保存しました"
                : "particles.yamlの保存に失敗しました";
    }

    ImGui::SameLine();

    if (ImGui::Button("YAML再読み込み")) {
        const bool loaded = particleSystem.ReloadDefinitions();
        particleSystem.Clear();

        if (loaded) {
            ResetSelectionIfInvalid(particleSystem);
            mStatusMessage = "particles.yamlを再読み込みしました";
        } else {
            mStatusMessage = "particles.yamlの再読み込みに失敗しました";
        }
    }

    ImGui::SameLine();

    if (ImGui::Button("全粒子を消去")) {
        particleSystem.Clear();
    }

    ImGui::Text(
        "粒子数: %zu / %zu",
        particleSystem.GetParticleCount(),
        particleSystem.GetMaxParticleCount());

    ImGui::TextDisabled(
        "保存先: %s",
        particleSystem.GetDefinitionFilePath().empty()
            ? "未設定"
            : particleSystem.GetDefinitionFilePath().c_str());
}

void ParticleEffectDebugPanel::DrawPreviewControls(ParticleSystem& particleSystem)
{
    ImGui::TextUnformatted("プレビュー");

    ImGui::Combo(
        "発生位置",
        &mPreviewPositionMode,
        previewPositionLabels,
        IM_ARRAYSIZE(previewPositionLabels));

    if (mPreviewPositionMode == 1) {
        ImGui::DragFloat(
            "正面までの距離",
            &mPreviewForwardDistance,
            0.05f,
            0.0f,
            50.0f,
            "%.2f");
    } else if (mPreviewPositionMode == 2) {
        ImGui::DragFloat3("任意座標", &mCustomPreviewPosition.x, 0.05f);
    }

    ImGui::DragFloat("全体スケール", &mPreviewScale, 0.02f, 0.0f, 20.0f, "%.2f");
    ImGui::Checkbox("プレイヤーの上下・正面方向を使う", &mUsePlayerAxes);

    if (!mUsePlayerAxes) {
        ImGui::DragFloat3("法線", &mPreviewNormal.x, 0.02f);
        ImGui::DragFloat3("放出方向", &mPreviewDirection.x, 0.02f);
    }

    ImGui::Checkbox("選択したEmitterだけ再生", &mPreviewSelectedEmitterOnly);

    if (ImGui::Button("1回再生")) {
        EmitPreview(particleSystem);
    }

    ImGui::SameLine();

    if (ImGui::Button("プレイヤー正面を任意座標へコピー")) {
        const ParticleSpawnContext context = BuildPreviewContext();
        mCustomPreviewPosition = context.position;
        mPreviewPositionMode = 2;
    }

    const bool autoPreviewChanged = ImGui::Checkbox("自動再生", &mAutoPreview);
    if (autoPreviewChanged && mAutoPreview) {
        mAutoPreviewTimer = 0.0f;
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150.0f);
    ImGui::DragFloat(
        "再生間隔",
        &mAutoPreviewInterval,
        0.02f,
        0.05f,
        10.0f,
        "%.2f秒");
}

void ParticleEffectDebugPanel::DrawEmitterControls(ParticleSystem& particleSystem)
{
    ImGui::TextUnformatted("Emitter");

    ParticleEffectDefinition* effectDefinition =
        mSelectedEffectId.empty()
            ? nullptr
            : particleSystem.FindEffectMutable(mSelectedEffectId);

    if (!effectDefinition) {
        ImGui::TextDisabled("エフェクトを選択してください");
        return;
    }

    for (int emitterIndex = 0;
         emitterIndex < static_cast<int>(effectDefinition->emitters.size());
         ++emitterIndex) {
        const ParticleEmitterDefinition& emitter = effectDefinition->emitters[emitterIndex];

        char label[256];
        std::snprintf(
            label,
            sizeof(label),
            "%02d  %s  count=%d",
            emitterIndex,
            emitter.texturePath.c_str(),
            emitter.count);

        if (ImGui::Selectable(label, emitterIndex == mSelectedEmitterIndex)) {
            SelectEmitter(particleSystem, emitterIndex);
        }
    }

    if (ImGui::Button("Emitter追加")) {
        effectDefinition->emitters.push_back(CreateDefaultEmitter());
        SelectEmitter(
            particleSystem,
            static_cast<int>(effectDefinition->emitters.size()) - 1);
        mStatusMessage = "Emitterを追加しました";
    }

    ImGui::SameLine();

    const bool hasSelectedEmitter =
        mSelectedEmitterIndex >= 0 &&
        mSelectedEmitterIndex < static_cast<int>(effectDefinition->emitters.size());

    if (!hasSelectedEmitter) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Emitter複製") && hasSelectedEmitter) {
        const ParticleEmitterDefinition copy =
            effectDefinition->emitters[mSelectedEmitterIndex];

        effectDefinition->emitters.push_back(copy);
        SelectEmitter(
            particleSystem,
            static_cast<int>(effectDefinition->emitters.size()) - 1);
        mStatusMessage = "Emitterを複製しました";
    }

    ImGui::SameLine();

    if (ImGui::Button("Emitter削除") && hasSelectedEmitter) {
        effectDefinition->emitters.erase(
            effectDefinition->emitters.begin() + mSelectedEmitterIndex);

        if (effectDefinition->emitters.empty()) {
            mSelectedEmitterIndex = -1;
        } else {
            SelectEmitter(
                particleSystem,
                std::min(
                    mSelectedEmitterIndex,
                    static_cast<int>(effectDefinition->emitters.size()) - 1));
        }

        mStatusMessage = "Emitterを削除しました";
    }

    if (!hasSelectedEmitter) {
        ImGui::EndDisabled();
        ImGui::TextDisabled("Emitterを選択してください");
        return;
    }

    ParticleEmitterDefinition& emitter =
        effectDefinition->emitters[mSelectedEmitterIndex];

    ImGui::Separator();
    ImGui::Text("Emitter %d の設定", mSelectedEmitterIndex);

    if (ImGui::InputText(
            "テクスチャ",
            mTexturePathBuffer,
            sizeof(mTexturePathBuffer))) {
        emitter.texturePath = mTexturePathBuffer;
    }

    int blendModeIndex = ToIndex(emitter.blendMode);
    if (ImGui::Combo(
            "ブレンド",
            &blendModeIndex,
            blendModeLabels,
            IM_ARRAYSIZE(blendModeLabels))) {
        emitter.blendMode = ToBlendMode(blendModeIndex);
    }

    int renderModeIndex = ToIndex(emitter.renderMode);
    if (ImGui::Combo(
            "描画方式",
            &renderModeIndex,
            renderModeLabels,
            IM_ARRAYSIZE(renderModeLabels))) {
        emitter.renderMode = ToRenderMode(renderModeIndex);
    }

    int directionModeIndex = ToIndex(emitter.directionMode);
    if (ImGui::Combo(
            "放出方向",
            &directionModeIndex,
            directionModeLabels,
            IM_ARRAYSIZE(directionModeLabels))) {
        emitter.directionMode = ToDirectionMode(directionModeIndex);
    }

    ImGui::DragInt("粒子数", &emitter.count, 1.0f, 0, 4096);
    emitter.count = std::max(0, emitter.count);

    DrawRange("寿命 min/max", emitter.lifetime, 0.01f, 0.001f, 60.0f, "%.3f");
    DrawRange("速度 min/max", emitter.speed, 0.02f, 0.0f, 100.0f, "%.2f");
    DrawRange("開始サイズ min/max", emitter.startSize, 0.01f, 0.0f, 100.0f, "%.3f");

    ImGui::DragFloat(
        "終了サイズ倍率",
        &emitter.endSizeMultiplier,
        0.01f,
        0.0f,
        20.0f,
        "%.3f");

    DrawRange(
        "初期回転 min/max",
        emitter.rotationDegrees,
        1.0f,
        -3600.0f,
        3600.0f,
        "%.1f");

    DrawRange(
        "回転速度 min/max",
        emitter.angularVelocityDegrees,
        1.0f,
        -3600.0f,
        3600.0f,
        "%.1f");

    if (emitter.directionMode == ParticleDirectionMode::Cone) {
        ImGui::DragFloat(
            "コーン角度",
            &emitter.spreadAngleDegrees,
            0.5f,
            0.0f,
            180.0f,
            "%.1f");
    }

    ImGui::DragFloat("重力", &emitter.gravity, 0.02f, -100.0f, 100.0f, "%.2f");
    ImGui::DragFloat("抵抗", &emitter.drag, 0.02f, 0.0f, 100.0f, "%.2f");
    emitter.drag = std::max(0.0f, emitter.drag);

    ImGui::DragFloat(
        "速度方向への伸び",
        &emitter.velocityStretch,
        0.02f,
        0.0f,
        100.0f,
        "%.2f");
    emitter.velocityStretch = std::max(0.0f, emitter.velocityStretch);

    ImGui::DragFloat3("発生位置オフセット", &emitter.positionOffset.x, 0.02f);
    ImGui::ColorEdit4("開始色", &emitter.startColor.x);
    ImGui::ColorEdit4("終了色", &emitter.endColor.x);

    if (ImGui::Button("このEmitterを1回再生")) {
        const ParticleSpawnContext context = BuildPreviewContext();
        particleSystem.EmitEmitter(emitter, context);
    }
}

void ParticleEffectDebugPanel::SelectEffect(
    ParticleSystem& particleSystem,
    const std::string& effectId)
{
    mSelectedEffectId = effectId;
    mSelectedEmitterIndex = -1;

    const ParticleEffectDefinition* definition =
        particleSystem.FindEffect(effectId);

    if (definition && !definition->emitters.empty()) {
        SelectEmitter(particleSystem, 0);
    }
}

void ParticleEffectDebugPanel::SelectEmitter(
    ParticleSystem& particleSystem,
    int emitterIndex)
{
    ParticleEffectDefinition* definition =
        particleSystem.FindEffectMutable(mSelectedEffectId);

    if (!definition ||
        emitterIndex < 0 ||
        emitterIndex >= static_cast<int>(definition->emitters.size())) {
        mSelectedEmitterIndex = -1;
        return;
    }

    mSelectedEmitterIndex = emitterIndex;
    SyncTexturePathBuffer(definition->emitters[emitterIndex]);
}

ParticleSpawnContext ParticleEffectDebugPanel::BuildPreviewContext() const
{
    ParticleSpawnContext context;
    context.position = mCustomPreviewPosition;
    context.normal = mPreviewNormal;
    context.direction = mPreviewDirection;
    context.scale = std::max(0.0f, mPreviewScale);

    if (!mContext.game) {
        return context;
    }

    const Player* player = mContext.game->GetMainPlayer();
    if (!player) {
        return context;
    }

    const glm::vec3 playerUp = player->GetUpVec();
    const glm::vec3 playerForward = player->GetFacingForwardVec();
    const float radius = std::max(0.1f, player->GetRadius());

    if (mPreviewPositionMode == 0) {
        context.position = player->GetPos() + playerUp * radius * 0.55f;
    } else if (mPreviewPositionMode == 1) {
        context.position =
            player->GetPos() +
            playerUp * radius * 0.55f +
            playerForward * mPreviewForwardDistance;
    }

    if (mUsePlayerAxes) {
        context.normal = playerUp;
        context.direction = playerForward;
    }

    return context;
}

void ParticleEffectDebugPanel::EmitPreview(ParticleSystem& particleSystem)
{
    if (mSelectedEffectId.empty()) {
        mStatusMessage = "エフェクトを選択してください";
        return;
    }

    const ParticleSpawnContext context = BuildPreviewContext();

    if (mPreviewSelectedEmitterOnly) {
        const ParticleEffectDefinition* definition =
            particleSystem.FindEffect(mSelectedEffectId);

        if (!definition ||
            mSelectedEmitterIndex < 0 ||
            mSelectedEmitterIndex >= static_cast<int>(definition->emitters.size())) {
            mStatusMessage = "Emitterを選択してください";
            return;
        }

        particleSystem.EmitEmitter(
            definition->emitters[mSelectedEmitterIndex],
            context);
        return;
    }

    if (!particleSystem.Emit(mSelectedEffectId, context)) {
        mStatusMessage = "エフェクトを再生できませんでした";
    }
}

void ParticleEffectDebugPanel::SyncTexturePathBuffer(
    const ParticleEmitterDefinition& emitter)
{
    std::strncpy(
        mTexturePathBuffer,
        emitter.texturePath.c_str(),
        sizeof(mTexturePathBuffer) - 1);

    mTexturePathBuffer[sizeof(mTexturePathBuffer) - 1] = '\0';
}

void ParticleEffectDebugPanel::ResetSelectionIfInvalid(
    ParticleSystem& particleSystem)
{
    if (mSelectedEffectId.empty()) {
        const std::vector<std::string> effectIds = particleSystem.GetEffectIds();
        if (!effectIds.empty()) {
            SelectEffect(particleSystem, effectIds.front());
        }
        return;
    }

    ParticleEffectDefinition* definition =
        particleSystem.FindEffectMutable(mSelectedEffectId);

    if (!definition) {
        mSelectedEffectId.clear();
        mSelectedEmitterIndex = -1;
        return;
    }

    if (definition->emitters.empty()) {
        mSelectedEmitterIndex = -1;
        return;
    }

    if (mSelectedEmitterIndex < 0 ||
        mSelectedEmitterIndex >= static_cast<int>(definition->emitters.size())) {
        SelectEmitter(particleSystem, 0);
    }
}
