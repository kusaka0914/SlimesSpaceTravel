#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <unordered_map>

// 音量は0～2の倍率（0～200%）。実際の音量はマスター×カテゴリ×音源ごとで決める。
struct AudioVolumeSettings {
    float master = 1.0f;
    float bgm = 80.0f / 128.0f;
    float se = 60.0f / 128.0f;
    std::unordered_map<std::string, float> bgmTracks;
    std::unordered_map<std::string, float> seTracks;

    static float Clamp(float value) { return std::isfinite(value) ? std::clamp(value, 0.0f, 2.0f) : 1.0f; }
    static float GetTrackVolume(const std::unordered_map<std::string, float>& tracks, const std::string& name)
    {
        const auto found = tracks.find(name);
        return found == tracks.end() ? 1.0f : Clamp(found->second);
    }
    void Sanitize()
    {
        master = Clamp(master);
        bgm = Clamp(bgm);
        se = Clamp(se);
        for (auto& [name, volume] : bgmTracks) {
            volume = Clamp(volume);
        }
        for (auto& [name, volume] : seTracks) {
            volume = Clamp(volume);
        }
    }
};
