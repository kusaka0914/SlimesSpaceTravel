#include "actor/OperationGuideZone.h"

#include "app/Game.h"
#include "utils/YamlRead.h"

#include <glm/gtc/quaternion.hpp>

OperationGuideZone::OperationGuideZone(Game* game)
    : StageObject(game)
{
    SetCollisionEnabled(false);
    SetModelPath("cameraZone.obj");
    SetScale(glm::vec3(6.0f));
}

float OperationGuideZone::GetRenderOpacity() const
{
    return mGame && mGame->GetIsDebugEditorShowing() ? 0.08f : 0.0f;
}

bool OperationGuideZone::Contains(const glm::vec3& worldPosition) const
{
    // CameraZoneと同じモデル軸補正で、Editorに表示される箱と判定を一致させる。
    const glm::vec3 semantic = glm::inverse(GetOrientation()) * (worldPosition - GetPos());
    const glm::vec3 local(-semantic.z, semantic.y, -semantic.x);
    return glm::all(glm::lessThanEqual(glm::abs(local), glm::abs(GetScale()) * 0.5f));
}

void OperationGuideZone::LoadGuide(const YAML::Node& node)
{
    const YAML::Node guide = node && node.IsMap() ? node : YAML::Node(YAML::NodeType::Map);
    enabled = YamlRead::ReadOr<bool>(guide, "enabled", true);
    priority = YamlRead::ReadOr<int>(guide, "priority", 0);
    operations.clear();
    const auto readOperation = [](const YAML::Node& entry) {
        Operation operation;
        operation.iconId = YamlRead::ReadOr<std::string>(entry, "iconId", "buttonA");
        operation.description = YamlRead::ReadOr<std::string>(entry, "description", "ジャンプ");
        if (operation.iconId == "buttonB_copy_copy2" && operation.description == "スライム切り替え") {
            operation.description = "分身切り替え";
        }
        operation.repeatInput = YamlRead::ReadOr<bool>(
            entry, "repeatInput", operation.iconId == "buttonA" && operation.description == "空中ダッシュ");
        return operation;
    };
    const YAML::Node entries = guide["operations"];
    if (entries && entries.IsSequence()) {
        for (const YAML::Node& entry : entries) {
            if (operations.size() >= MaximumOperations) {
                break;
            }
            if (entry.IsMap()) {
                operations.push_back(readOperation(entry));
            }
        }
    } else {
        // 従来の1件形式も読み込み、次回保存時に複数件形式へ移行する。
        operations.push_back(readOperation(guide));
    }
}

void OperationGuideZone::SaveGuide(YAML::Node node) const
{
    node["enabled"] = enabled;
    node["priority"] = priority;
    YAML::Node entries(YAML::NodeType::Sequence);
    for (const Operation& operation : operations) {
        if (entries.size() >= MaximumOperations) {
            break;
        }
        YAML::Node entry;
        entry["iconId"] = operation.iconId;
        entry["description"] = operation.description;
        entry["repeatInput"] = operation.repeatInput;
        entries.push_back(entry);
    }
    node["operations"] = entries;
    node.remove("iconId");
    node.remove("description");
    node.remove("repeatInput");
}
