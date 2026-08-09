#pragma once

#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "imgui.h"

#include <string>

namespace EditorAssetDragDrop {

inline const char* ResolvePayloadType(EditorAssetType assetType)
{
    switch (assetType) {
    case EditorAssetType::Model:
        return "EDITOR_MODEL_ASSET";
    case EditorAssetType::Texture:
        return "EDITOR_TEXTURE_ASSET";
    case EditorAssetType::Video:
        return "EDITOR_VIDEO_ASSET";
    case EditorAssetType::Count:
        return "EDITOR_UNKNOWN_ASSET";
    }

    return "EDITOR_UNKNOWN_ASSET";
}

inline void SetPayload(
    EditorAssetType assetType,
    const std::string& assetRelativePath)
{
    ImGui::SetDragDropPayload(
        ResolvePayloadType(assetType),
        assetRelativePath.c_str(),
        assetRelativePath.size() + 1);
}

inline bool AcceptPath(
    EditorAssetType assetType,
    std::string& acceptedAssetRelativePath)
{
    if (!ImGui::BeginDragDropTarget()) {
        return false;
    }

    const ImGuiPayload* payload =
        ImGui::AcceptDragDropPayload(ResolvePayloadType(assetType));
    if (!payload || !payload->Data || payload->DataSize <= 1) {
        ImGui::EndDragDropTarget();
        return false;
    }

    const char* payloadText = static_cast<const char*>(payload->Data);
    const bool hasNullTerminator =
        payloadText[payload->DataSize - 1] == '\0';
    if (!hasNullTerminator) {
        ImGui::EndDragDropTarget();
        return false;
    }

    acceptedAssetRelativePath.assign(
        payloadText,
        static_cast<std::size_t>(payload->DataSize - 1));
    ImGui::EndDragDropTarget();
    return true;
}

} // namespace EditorAssetDragDrop
