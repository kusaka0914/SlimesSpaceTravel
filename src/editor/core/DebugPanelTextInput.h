#pragma once

#include "imgui.h"

#include <array>
#include <cstdio>
#include <string>

namespace DebugPanelTextInput {

// BufferSizeは終端を含むバイト数。入力が変更された場合だけtextを更新し、trueを返す。
template <std::size_t BufferSize>
bool DrawStringInput(const char* label, std::string& text, ImGuiInputTextFlags flags = 0)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputText(label, buffer.data(), buffer.size(), flags)) {
        return false;
    }
    text = buffer.data();
    return true;
}

template <std::size_t BufferSize>
bool DrawMultilineStringInput(const char* label, std::string& text, const ImVec2& size)
{
    std::array<char, BufferSize> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
    if (!ImGui::InputTextMultiline(label, buffer.data(), buffer.size(), size)) {
        return false;
    }
    text = buffer.data();
    return true;
}

} // namespace DebugPanelTextInput
