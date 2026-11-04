#include "editor/tools/parameters/AudioParameterDebugPanel.h"

#include "app/Game.h"
#include "imgui.h"
#include "system/AudioSystem.h"

#include <string_view>
#include <utility>

namespace {
std::string_view GetAudioLabel(std::string_view name)
{
    static constexpr std::pair<std::string_view, std::string_view> labels[] = {
        {"stage_bgm", "ステージ"},
        {"boss_bgm", "ボス戦"},
        {"title_bgm", "タイトル"},
        {"opening_bgm", "オープニング"},
        {"base_bgm", "拠点"},
        {"enter_stage_bgm", "ステージ進入"},
        {"star_wait_bgm", "スター待機"},
        {"attack_se", "攻撃命中"},
        {"attack_miss_se", "空振り"},
        {"attack_pre_se", "敵の攻撃予兆"},
        {"counter_se", "カウンター"},
        {"clear_se", "クリア"},
        {"attack_air_se", "空中攻撃"},
        {"defeat_se", "撃破"},
        {"all_kill", "敵全滅"},
        {"damaged_se", "被弾"},
        {"destroy_se", "ガード区画破壊"},
        {"break_se", "ガード全破壊"},
        {"air_charged_se", "空中強攻撃開始"},
        {"show_boat_se", "ロケット出現"},
        {"show_key_se", "鍵出現"},
        {"pickup_se", "アイテム取得"},
        {"dodge_se", "ダッシュ"},
        {"jump_se", "ジャンプ"},
        {"recover_se", "回復"},
        {"message_se", "メッセージ"},
        {"just_attack_se", "溜め攻撃カウンター成功"},
        {"just_dodge_se", "ダッシュカウンター成功"},
        {"charging_se", "溜め1段階目"},
        {"charged_se", "最大溜め完了"},
        {"boss_defeated_se", "ボス撃破"},
        {"star_shown_se", "スター出現"},
    };
    for (const auto& [id, label] : labels) {
        if (name == id) {
            return label;
        }
    }
    return name;
}

bool DrawVolumeSlider(const char* label, float& volume)
{
    float percent = volume * 100.0f;
    if (!ImGui::SliderFloat(label, &percent, 0.0f, 200.0f, "%.0f%%")) {
        return false;
    }
    volume = percent / 100.0f;
    return true;
}
} // namespace

AudioParameterDebugPanel::AudioParameterDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void AudioParameterDebugPanel::Draw()
{
    AudioSystem* audio = mContext.game ? mContext.game->GetAudioSystem() : nullptr;
    if (!audio) {
        ImGui::TextDisabled("音声システムがありません。");
        return;
    }
    ImGui::TextUnformatted("音量調整");
    if (ImGui::Button("音量設定を保存")) {
        std::string error;
        mStatus = audio->SaveVolumeSettings(error) ? "音量設定を保存しました。" : "保存失敗: " + error;
    }
    ImGui::SameLine();
    if (ImGui::Button("保存済み設定を再読込")) {
        std::string error;
        mStatus = audio->LoadVolumeSettings(error) ? "音量設定を再読込しました。" : "読込失敗: " + error;
    }
    ImGui::SameLine();
    if (ImGui::Button("試聴を停止")) {
        audio->StopPreview();
    }
    if (!mStatus.empty()) {
        ImGui::TextWrapped("%s", mStatus.c_str());
    }
    ImGui::TextDisabled("変更は即時反映。保存すると次回起動時も適用されます。");
    ImGui::TextDisabled("100%以上は音を増幅します。大きくすると音が歪む場合があります。");
    ImGui::TextDisabled("BGMの試聴終了・停止後は、試聴前の曲を先頭から再開します。");

    auto settings = audio->GetVolumeSettings();
    bool changed = false;
    changed |= DrawVolumeSlider("マスター", settings.master);
    changed |= DrawVolumeSlider("BGM全体", settings.bgm);
    changed |= DrawVolumeSlider("SE全体", settings.se);
    // 同じフレームの試聴にも、直前に動かした全体音量を適用する。
    if (changed) {
        audio->SetVolumeSettings(settings);
    }
    ImGui::InputText("名前・IDで検索", mSearch, sizeof(mSearch));
    const auto drawTracks = [&](const char* title, const auto& names, auto& volumes, bool isBGM) {
        if (!ImGui::CollapsingHeader(title, ImGuiTreeNodeFlags_DefaultOpen)) {
            return;
        }
        ImGui::PushID(title);
        if (!ImGui::BeginTable("Tracks", 3, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_RowBg)) {
            ImGui::PopID();
            return;
        }
        ImGui::TableSetupColumn("音源", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("音量", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("試聴", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();
        for (const auto& name : names) {
            const auto label = GetAudioLabel(name);
            if (std::string_view(name).find(mSearch) == std::string_view::npos &&
                label.find(mSearch) == std::string_view::npos) {
                continue;
            }
            ImGui::PushID(name.c_str());
            float volume = AudioVolumeSettings::GetTrackVolume(volumes, name);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(label.data(), label.data() + label.size());
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", name.c_str());
            }
            ImGui::TableSetColumnIndex(1);
            ImGui::SetNextItemWidth(-1.0f);
            if (DrawVolumeSlider("##volume", volume)) {
                volumes[name] = volume;
                audio->SetVolumeSettings(settings);
            }
            ImGui::TableSetColumnIndex(2);
            if (ImGui::Button("試聴")) {
                if (isBGM) {
                    audio->PreviewBGM(name);
                } else {
                    audio->PreviewSE(name);
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
        ImGui::PopID();
    };
    drawTracks("BGM別", audio->GetBGMNames(), settings.bgmTracks, true);
    drawTracks("SE別", audio->GetSENames(), settings.seTracks, false);
}
