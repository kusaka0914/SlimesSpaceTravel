#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <string>

enum class UICanvasOperation {
    Translate,
    Rotate,
    Scale,
};

enum class UICanvasSelectionSource {
    None,
    Custom,
    ExistingTexture,
    ExistingText,
};

// Customは要素配列の添字、既存UIはscreen.id形式のキーで識別する。UI要素は所有しない。
struct UICanvasElementReference {
    UICanvasSelectionSource source = UICanvasSelectionSource::None;
    std::size_t customIndex = 0;
    std::string existingKey;

    bool operator==(const UICanvasElementReference& other) const = default;
};

// 左上原点のフレームバッファ座標。center・sizeはピクセル単位、回転は度。
struct UICanvasElementTransform {
    glm::vec2 center = glm::vec2(0.0f);
    glm::vec2 size = glm::vec2(1.0f);
    float rotationDegrees = 0.0f;
};
