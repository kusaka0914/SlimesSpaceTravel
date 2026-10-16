#pragma once

#include <string>

// 配置対象の識別子。シーケンス名とYAML添字の組、またはplatformIdで指定する。
// IsValidは指定形式だけを確認し、対象の存在は確認しない。
struct PlatformRevealTarget {
    std::string sequenceName;
    int yamlIndex = -1;
    std::string platformId;

    bool IsValid() const { return !platformId.empty() || (!sequenceName.empty() && yamlIndex >= 0); }
};
