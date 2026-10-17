#pragma once

#include "effect/particle/ParticleTypes.h"

#include <cstddef>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// エフェクト定義と生成済みParticleを保持し、生成数を制限して寿命・移動を更新する。
class ParticleSystem {
public:
    explicit ParticleSystem(std::size_t maxParticleCount = 4096);

    // 読込成功時だけ定義と再読込先を置き換える。失敗時は現在の定義を維持してfalseを返す。
    bool LoadDefinitions(const std::string& filePath);
    bool ReloadDefinitions();
    bool SaveDefinitions() const;

    // 未登録IDならfalse。trueは定義の存在を表し、生成上限などによって実際の生成数は0にもなる。
    bool Emit(const std::string& effectId, const ParticleSpawnContext& context);

    // 生成上限の範囲でEmitter定義を直接生成する。エフェクト定義への登録は行わない。
    void EmitEmitter(const ParticleEmitterDefinition& definition, const ParticleSpawnContext& context);

    // メモリ上に定義を追加する。空ID・重複IDはfalse。ファイル保存は別途行う。
    bool CreateEffect(const std::string& effectId);

    // 指定定義を追加する。空ID・重複IDはfalse。ファイル保存は別途行う。
    bool CreateEffect(const std::string& effectId, const ParticleEffectDefinition& definition);
    bool RemoveEffect(std::string_view effectId);

    // 定義への借用ポインターを返す。未登録ならnullptr。削除・定義再読込で無効になる。
    const ParticleEffectDefinition* FindEffect(std::string_view effectId) const;

    // 編集用の借用ポインターを返す。未登録ならnullptr。削除・定義再読込で無効になる。
    ParticleEffectDefinition* FindEffectMutable(std::string_view effectId);
    std::vector<std::string> GetEffectIds() const;

    // 秒単位で移動と寿命を更新し、寿命切れのParticleを削除する。deltaTimeが0以下なら何もしない。
    void Update(float deltaTime);

    // 生成済みParticleだけを削除する。エフェクト定義は保持する。
    void Clear();

    const std::string& GetDefinitionFilePath() const { return mDefinitionFilePath; }
    const std::vector<Particle>& GetParticles() const { return mParticles; }
    std::size_t GetParticleCount() const { return mParticles.size(); }
    std::size_t GetMaxParticleCount() const { return mMaxParticleCount; }

private:
    void EmitFromDefinition(const ParticleEmitterDefinition& definition, const ParticleSpawnContext& context);

    float RandomFloat(const ParticleFloatRange& range);
    glm::vec3 CreateSpawnDirection(const ParticleEmitterDefinition& definition, const ParticleSpawnContext& context);
    glm::vec3 RandomUnitVector();
    glm::vec3 RandomHemisphereDirection(const glm::vec3& normal);
    glm::vec3 RandomConeDirection(const glm::vec3& axis, float spreadAngleRadians);

    std::unordered_map<std::string, ParticleEffectDefinition> mDefinitions;
    std::vector<Particle> mParticles;
    std::mt19937 mRandomEngine;
    std::size_t mMaxParticleCount;
    std::string mDefinitionFilePath;
};
