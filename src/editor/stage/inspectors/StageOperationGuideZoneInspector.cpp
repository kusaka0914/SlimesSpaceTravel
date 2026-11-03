#include "editor/stage/inspectors/StageOperationGuideZoneInspector.h"

#include "actor/OperationGuideZone.h"

#include <array>
#include <cstddef>
#include <cstdio>
#include <imgui.h>

bool DrawOperationGuideZoneInspector(OperationGuideZone& zone)
{
    ImGui::SeparatorText("操作ガイドゾーン");
    bool changed = ImGui::Checkbox("有効", &zone.enabled);
    changed |= ImGui::InputInt("優先度", &zone.priority);
    ImGui::TextDisabled("重なった場合は優先度が高いゾーンから表示枠を使います。同値なら一覧順です。");

    struct Operation {
        const char* label;
        const char* iconId;
        const char* description;
        bool repeatInput = false;
    };
    constexpr Operation operations[] = {
        {"ジャンプ", "buttonA", "ジャンプ"},
        {"空中ダッシュ（A → A）", "buttonA", "空中ダッシュ", true},
        {"ダッシュ", "buttonB", "ダッシュ"},
        {"攻撃", "buttonY", "攻撃"},
        {"弱攻撃", "buttonY", "弱攻撃"},
        {"連続攻撃", "buttonY", "長押しで連続攻撃"},
        {"強攻撃", "buttonX", "強攻撃"},
        {"溜め攻撃", "buttonX", "長押しで溜め攻撃"},
        {"回復", "buttonB_copy_copy2_copy2", "回復"},
        {"カメラリセット", "buttonB_copy_copy_copy", "カメラリセット"},
        {"分裂", "buttonB_copy_copy2_copy", "分裂"},
        {"合体", "buttonB_copy_copy2_copy", "合体"},
        {"分裂・合体", "buttonB_copy_copy2_copy", "分裂・合体"},
        {"分身切り替え", "buttonB_copy_copy2", "分身切り替え"},
        {"移動", "buttonB_copy", "移動"},
        {"カメラ回転", "buttonB_copy_copy", "カメラ回転"},
    };
    ImGui::TextDisabled("操作ごとに画像と説明文がセットになります。最大3つまで縦に表示します。");
    for (std::size_t index = 0; index < zone.operations.size();) {
        ImGui::PushID(static_cast<int>(index));
        ImGui::SeparatorText(("操作 " + std::to_string(index + 1)).c_str());
        auto& entry = zone.operations[index];
        const char* selectedLabel = "カスタム";
        for (const auto& operation : operations) {
            if (entry.iconId == operation.iconId && entry.description == operation.description &&
                entry.repeatInput == operation.repeatInput) {
                selectedLabel = operation.label;
                break;
            }
        }
        if (ImGui::BeginCombo("表示する操作", selectedLabel)) {
            for (const auto& operation : operations) {
                const bool selected = entry.iconId == operation.iconId && entry.description == operation.description &&
                                      entry.repeatInput == operation.repeatInput;
                if (ImGui::Selectable(operation.label, selected)) {
                    entry.iconId = operation.iconId;
                    entry.description = operation.description;
                    entry.repeatInput = operation.repeatInput;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }

        std::array<char, 512> description{};
        std::snprintf(description.data(), description.size(), "%s", entry.description.c_str());
        if (ImGui::InputText("説明文（任意調整）", description.data(), description.size())) {
            entry.description = description.data();
            changed = true;
        }
        if (ImGui::Button("この操作を削除")) {
            zone.operations.erase(zone.operations.begin() + static_cast<std::ptrdiff_t>(index));
            changed = true;
            ImGui::PopID();
            break;
        }
        ++index;
        ImGui::PopID();
    }
    if (zone.operations.size() < OperationGuideZone::MaximumOperations && ImGui::Button("操作を追加")) {
        zone.operations.emplace_back();
        changed = true;
    }
    ImGui::TextDisabled("画像は入力デバイスに合わせて切り替わります。ゾーンを出たら案内は消えます。");
    return changed;
}
