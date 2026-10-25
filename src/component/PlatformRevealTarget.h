#pragma once

#include <string>

// 配置対象の識別子。シーケンス名とYAML添字の組、またはplatformIdで指定する。
// IsValidは指定形式だけを確認し、対象の存在は確認しない。
struct PlatformRevealTarget {
    std::string sequenceName;
    int yamlIndex = -1;
    std::string platformId;

    bool IsValid() const { return !platformId.empty() || (!sequenceName.empty() && yamlIndex >= 0); }

    // 両方にIDがあればIDを優先し、それ以外は有効なシーケンス名と添字で照合する。
    bool RefersToSameActor(const PlatformRevealTarget& other) const
    {
        if (!platformId.empty() && !other.platformId.empty()) {
            return platformId == other.platformId;
        }
        return !sequenceName.empty() && yamlIndex >= 0 && sequenceName == other.sequenceName &&
               yamlIndex == other.yamlIndex;
    }
};
