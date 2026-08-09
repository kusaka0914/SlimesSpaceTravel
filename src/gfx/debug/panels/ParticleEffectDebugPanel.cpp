#include "gfx/debug/panels/ParticleEffectDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "Game.h"
#include "actor/Player.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "imgui.h"
#include "system/ParticleSystem.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
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

constexpr std::string_view particleTextureAssetPrefix =
    "textures/particles/";

template <std::size_t BufferSize>
bool DrawStringInput(const char* label, std::string& text)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputText(label, buffer.data(), buffer.size())) {
        return false;
    }

    text = buffer.data();
    return true;
}

std::string ToLower(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return text;
}

bool TryResolveParticleTexturePath(
    std::string_view assetPath,
    std::string& outParticleTexturePath)
{
    if (!assetPath.starts_with(particleTextureAssetPrefix)) {
        return false;
    }

    outParticleTexturePath =
        assetPath.substr(particleTextureAssetPrefix.size());
    return !outParticleTexturePath.empty();
}

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

    DrawToolbar(particleSystem);
    ImGui::Separator();
    DrawEffectList(particleSystem);
    ImGui::SameLine();
    DrawEffectEditor(particleSystem);

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

}

void ParticleEffectDebugPanel::DrawToolbar(ParticleSystem& particleSystem)
{
    if (ImGui::Button("YAMLへ保存")) {
        mStatusMessage =
            particleSystem.SaveDefinitions()
                ? "particles.yamlへ保存しました"
                : "particles.yamlの保存に失敗しました";
    }

    ImGui::SameLine();

    if (ImGui::Button("再読込")) {
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

    ImGui::SameLine();
    ImGui::TextDisabled(
        "粒子数 %zu / %zu",
        particleSystem.GetParticleCount(),
        particleSystem.GetMaxParticleCount());

    if (!mStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mStatusMessage.c_str());
    }
}

void ParticleEffectDebugPanel::DrawEffectList(
    ParticleSystem& particleSystem)
{
    ImGui::BeginChild(
        "ParticleEffectList",
        ImVec2(210.0f, 0.0f),
        true);
    ImGui::TextUnformatted("パーティクル一覧");

    ImGui::InputTextWithHint(
        "##NewParticleEffectId",
        "新しいエフェクトID",
        mNewEffectIdBuffer,
        sizeof(mNewEffectIdBuffer));

    if (ImGui::Button("新規作成", ImVec2(-1.0f, 0.0f))) {
        const std::string newEffectId(mNewEffectIdBuffer);
        if (particleSystem.CreateEffect(newEffectId)) {
            SelectEffect(particleSystem, newEffectId);
            mStatusMessage = "エフェクトを作成しました";
        } else {
            mStatusMessage = "IDが空、または同じIDが存在します";
        }
    }

    const ParticleEffectDefinition* selectedDefinition =
        mSelectedEffectId.empty()
            ? nullptr
            : particleSystem.FindEffect(mSelectedEffectId);
    if (!selectedDefinition) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("選択中を複製", ImVec2(-1.0f, 0.0f)) &&
        selectedDefinition) {
        const std::string newEffectId(mNewEffectIdBuffer);
        if (particleSystem.CreateEffect(newEffectId, *selectedDefinition)) {
            SelectEffect(particleSystem, newEffectId);
            mStatusMessage = "エフェクトを複製しました";
        } else {
            mStatusMessage = "複製先IDが空、または同じIDが存在します";
        }
    }

    if (ImGui::Button("選択中を削除", ImVec2(-1.0f, 0.0f)) &&
        selectedDefinition) {
        if (particleSystem.RemoveEffect(mSelectedEffectId)) {
            mSelectedEffectId.clear();
            mSelectedEmitterIndex = -1;
            mStatusMessage = "エフェクトを削除しました";
        }
    }

    if (!selectedDefinition) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
    for (const std::string& effectId : particleSystem.GetEffectIds()) {
        const ParticleEffectDefinition* definition =
            particleSystem.FindEffect(effectId);
        const std::string displayName =
            definition && !definition->displayName.empty()
                ? definition->displayName
                : effectId;
        const std::string label = displayName + "##" + effectId;
        if (ImGui::Selectable(
                label.c_str(),
                effectId == mSelectedEffectId)) {
            SelectEffect(particleSystem, effectId);
        }
        ImGui::TextDisabled("ID: %s", effectId.c_str());
    }

    ImGui::EndChild();
}

void ParticleEffectDebugPanel::DrawEffectEditor(
    ParticleSystem& particleSystem)
{
    ImGui::BeginChild(
        "ParticleEffectEditor",
        ImVec2(0.0f, 0.0f),
        true);

    ParticleEffectDefinition* definition =
        mSelectedEffectId.empty()
            ? nullptr
            : particleSystem.FindEffectMutable(mSelectedEffectId);
    if (!definition) {
        ImGui::TextWrapped(
            "左側で編集するパーティクルを選択してください。");
        ImGui::EndChild();
        return;
    }

    ImGui::Text("ID: %s", mSelectedEffectId.c_str());
    ImGui::TextDisabled(
        "IDはコードから参照されるため、作成後は固定です。");
    DrawStringInput<256>("表示名", definition->displayName);

    ImGui::TextDisabled(
        "保存先: %s",
        particleSystem.GetDefinitionFilePath().empty()
            ? "未設定"
            : particleSystem.GetDefinitionFilePath().c_str());

    if (ImGui::CollapsingHeader(
            "プレビュー",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        DrawPreviewControls(particleSystem);
    }

    ImGui::SeparatorText("Emitter");
    DrawEmitterList(particleSystem);
    DrawEmitterInspector(particleSystem);

    ImGui::EndChild();
}

void ParticleEffectDebugPanel::DrawPreviewControls(ParticleSystem& particleSystem)
{
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

void ParticleEffectDebugPanel::DrawEmitterList(
    ParticleSystem& particleSystem)
{
    ParticleEffectDefinition* effectDefinition =
        mSelectedEffectId.empty()
            ? nullptr
            : particleSystem.FindEffectMutable(mSelectedEffectId);

    if (!effectDefinition) {
        ImGui::TextDisabled("エフェクトを選択してください");
        return;
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
    }

    if (hasSelectedEmitter) {
        if (mSelectedEmitterIndex > 0 &&
            ImGui::Button("上へ移動")) {
            std::swap(
                effectDefinition->emitters[mSelectedEmitterIndex],
                effectDefinition->emitters[mSelectedEmitterIndex - 1]);
            --mSelectedEmitterIndex;
            SyncTexturePathBuffer(
                effectDefinition->emitters[mSelectedEmitterIndex]);
        }

        ImGui::SameLine();
        if (mSelectedEmitterIndex + 1 <
                static_cast<int>(effectDefinition->emitters.size()) &&
            ImGui::Button("下へ移動")) {
            std::swap(
                effectDefinition->emitters[mSelectedEmitterIndex],
                effectDefinition->emitters[mSelectedEmitterIndex + 1]);
            ++mSelectedEmitterIndex;
            SyncTexturePathBuffer(
                effectDefinition->emitters[mSelectedEmitterIndex]);
        }
    }

    ImGui::BeginChild(
        "ParticleEmitterList",
        ImVec2(0.0f, 135.0f),
        true);
    for (int emitterIndex = 0;
         emitterIndex < static_cast<int>(effectDefinition->emitters.size());
         ++emitterIndex) {
        const ParticleEmitterDefinition& emitter =
            effectDefinition->emitters[emitterIndex];

        char label[256];
        std::snprintf(
            label,
            sizeof(label),
            "%02d  %s  粒子数=%d",
            emitterIndex + 1,
            emitter.texturePath.c_str(),
            emitter.count);

        if (ImGui::Selectable(
                label,
                emitterIndex == mSelectedEmitterIndex)) {
            SelectEmitter(particleSystem, emitterIndex);
        }
    }
    ImGui::EndChild();
}

void ParticleEffectDebugPanel::DrawEmitterInspector(
    ParticleSystem& particleSystem)
{
    ParticleEffectDefinition* effectDefinition =
        mSelectedEffectId.empty()
            ? nullptr
            : particleSystem.FindEffectMutable(mSelectedEffectId);
    const bool hasSelectedEmitter =
        effectDefinition &&
        mSelectedEmitterIndex >= 0 &&
        mSelectedEmitterIndex <
            static_cast<int>(effectDefinition->emitters.size());
    if (!hasSelectedEmitter) {
        ImGui::TextDisabled("一覧からEmitterを選択してください");
        return;
    }

    ParticleEmitterDefinition& emitter =
        effectDefinition->emitters[mSelectedEmitterIndex];

    ImGui::Separator();
    ImGui::Text("Emitter %d の設定", mSelectedEmitterIndex + 1);
    ImGui::SameLine();
    if (ImGui::Button("このEmitterを1回再生")) {
        const ParticleSpawnContext context = BuildPreviewContext();
        particleSystem.EmitEmitter(emitter, context);
    }

    bool emitterChanged = false;

    if (ImGui::CollapsingHeader(
            "基本",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        emitterChanged |= DrawTexturePicker(emitter);

        int blendModeIndex = ToIndex(emitter.blendMode);
        if (ImGui::Combo(
                "ブレンド",
                &blendModeIndex,
                blendModeLabels,
                IM_ARRAYSIZE(blendModeLabels))) {
            emitter.blendMode = ToBlendMode(blendModeIndex);
            emitterChanged = true;
        }

        int renderModeIndex = ToIndex(emitter.renderMode);
        if (ImGui::Combo(
                "描画方式",
                &renderModeIndex,
                renderModeLabels,
                IM_ARRAYSIZE(renderModeLabels))) {
            emitter.renderMode = ToRenderMode(renderModeIndex);
            emitterChanged = true;
        }
    }

    if (ImGui::CollapsingHeader(
            "発生",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        int directionModeIndex = ToIndex(emitter.directionMode);
        if (ImGui::Combo(
                "放出方向",
                &directionModeIndex,
                directionModeLabels,
                IM_ARRAYSIZE(directionModeLabels))) {
            emitter.directionMode = ToDirectionMode(directionModeIndex);
            emitterChanged = true;
        }

        emitterChanged |=
            ImGui::DragInt("粒子数", &emitter.count, 1.0f, 0, 4096);
        emitter.count = std::max(0, emitter.count);

        emitterChanged |= DrawRange(
            "寿命 min/max",
            emitter.lifetime,
            0.01f,
            0.001f,
            60.0f,
            "%.3f");
        emitterChanged |= DrawRange(
            "速度 min/max",
            emitter.speed,
            0.02f,
            0.0f,
            100.0f,
            "%.2f");
        emitterChanged |= ImGui::DragFloat3(
            "発生位置オフセット",
            &emitter.positionOffset.x,
            0.02f);

        if (emitter.directionMode == ParticleDirectionMode::Cone) {
            emitterChanged |= ImGui::DragFloat(
                "コーン角度",
                &emitter.spreadAngleDegrees,
                0.5f,
                0.0f,
                180.0f,
                "%.1f");
        }
    }

    if (ImGui::CollapsingHeader(
            "移動",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        emitterChanged |= ImGui::DragFloat(
            "重力",
            &emitter.gravity,
            0.02f,
            -100.0f,
            100.0f,
            "%.2f");
        emitterChanged |= ImGui::DragFloat(
            "抵抗",
            &emitter.drag,
            0.02f,
            0.0f,
            100.0f,
            "%.2f");
        emitter.drag = std::max(0.0f, emitter.drag);
    }

    if (ImGui::CollapsingHeader(
            "見た目",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        emitterChanged |= DrawRange(
            "開始サイズ min/max",
            emitter.startSize,
            0.01f,
            0.0f,
            100.0f,
            "%.3f");
        emitterChanged |= ImGui::DragFloat(
            "終了サイズ倍率",
            &emitter.endSizeMultiplier,
            0.01f,
            0.0f,
            20.0f,
            "%.3f");
        emitterChanged |= DrawRange(
            "初期回転 min/max",
            emitter.rotationDegrees,
            1.0f,
            -3600.0f,
            3600.0f,
            "%.1f");
        emitterChanged |= DrawRange(
            "回転速度 min/max",
            emitter.angularVelocityDegrees,
            1.0f,
            -3600.0f,
            3600.0f,
            "%.1f");
        emitterChanged |= ImGui::DragFloat(
            "速度方向への伸び",
            &emitter.velocityStretch,
            0.02f,
            0.0f,
            100.0f,
            "%.2f");
        emitter.velocityStretch =
            std::max(0.0f, emitter.velocityStretch);
    }

    if (ImGui::CollapsingHeader(
            "色",
            ImGuiTreeNodeFlags_DefaultOpen)) {
        emitterChanged |=
            ImGui::ColorEdit4("開始色", &emitter.startColor.x);
        emitterChanged |=
            ImGui::ColorEdit4("終了色", &emitter.endColor.x);
    }

    if (emitterChanged && mAutoPreview) {
        mAutoPreviewTimer = 0.0f;
    }
}

bool ParticleEffectDebugPanel::DrawTexturePicker(
    ParticleEmitterDefinition& emitter)
{
    bool changed = false;

    if (ImGui::InputText(
            "テクスチャ",
            mTexturePathBuffer,
            sizeof(mTexturePathBuffer))) {
        emitter.texturePath = mTexturePathBuffer;
        changed = true;
    }

    std::string droppedTextureAssetPath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTextureAssetPath)) {
        std::string particleTexturePath;
        if (TryResolveParticleTexturePath(
                droppedTextureAssetPath,
                particleTexturePath)) {
            emitter.texturePath = particleTexturePath;
            SyncTexturePathBuffer(emitter);
            changed = true;
        } else {
            mStatusMessage =
                "particlesフォルダー内の画像を選択してください";
        }
    }

    ImGui::Button(
        "画像アセットをここへドロップ##particleTextureDrop",
        ImVec2(-1.0f, 0.0f));
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTextureAssetPath)) {
        std::string particleTexturePath;
        if (TryResolveParticleTexturePath(
                droppedTextureAssetPath,
                particleTexturePath)) {
            emitter.texturePath = particleTexturePath;
            SyncTexturePathBuffer(emitter);
            changed = true;
        } else {
            mStatusMessage =
                "particlesフォルダー内の画像を選択してください";
        }
    }

    if (mContext.assetCatalog) {
        mContext.assetCatalog->EnsureScanned();

        ImGui::InputTextWithHint(
            "##ParticleTextureFilter",
            "パーティクル画像を検索",
            mTextureAssetFilter.data(),
            mTextureAssetFilter.size());

        const std::string filter = ToLower(mTextureAssetFilter.data());
        if (ImGui::BeginCombo(
                "画像アセット",
                emitter.texturePath.c_str())) {
            for (const std::string& assetPath :
                 mContext.assetCatalog->GetPaths(
                     EditorAssetType::Texture)) {
                std::string particleTexturePath;
                if (!TryResolveParticleTexturePath(
                        assetPath,
                        particleTexturePath)) {
                    continue;
                }
                if (!filter.empty() &&
                    ToLower(particleTexturePath).find(filter) ==
                        std::string::npos) {
                    continue;
                }

                if (ImGui::Selectable(
                        particleTexturePath.c_str(),
                        particleTexturePath == emitter.texturePath)) {
                    emitter.texturePath = particleTexturePath;
                    SyncTexturePathBuffer(emitter);
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
    }

    if (mContext.uiRenderer && !emitter.texturePath.empty()) {
        const std::string assetPath =
            std::string(particleTextureAssetPrefix) +
            emitter.texturePath;
        if (mContext.uiRenderer->RegisterCustomUITexture(assetPath)) {
            const unsigned int texture =
                mContext.uiRenderer->GetCustomUITextureHandle(assetPath);
            if (texture != 0) {
                ImGui::TextUnformatted("テクスチャプレビュー");
                ImGui::Image(
                    static_cast<ImTextureID>(texture),
                    ImVec2(96.0f, 96.0f),
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f));
            }
        }
    }

    return changed;
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
