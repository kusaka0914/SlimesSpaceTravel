#pragma once

#include "gfx/debug/DebugEditorContext.h"

#include <glm/glm.hpp>

class UGCEditorTutorial;

class UGCEditorViewController {
public:
    UGCEditorViewController(
        DebugEditorContext& context,
        UGCEditorTutorial& editorTutorial);

    void AdjustZoom(float distanceMultiplier);
    void ToggleVerticalView();
    void SetFixedView(const glm::vec3& viewDirection);
    const glm::vec3& GetViewDirection() const;

private:
    void AdjustViewDistance(float distanceMultiplier);

    DebugEditorContext& mContext;
    UGCEditorTutorial& mEditorTutorial;
    glm::vec3 mViewDirection{0.0f, 1.0f, 0.0f};
};
