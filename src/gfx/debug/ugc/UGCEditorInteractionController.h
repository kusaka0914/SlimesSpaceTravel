#pragma once

#include <glm/glm.hpp>

class UGCEditCommandController;
class UGCEditLayerController;
class UGCEditorViewController;
class UGCSceneInteractionController;

class UGCEditorInteractionController {
public:
    UGCEditorInteractionController(
        UGCEditCommandController& editCommandController,
        UGCEditLayerController& editLayerController,
        UGCEditorViewController& viewController,
        UGCSceneInteractionController& sceneInteractionController);

    void HandleUndo();
    void HandleRedo();
    void ToggleEraser();
    void ActivateSelectionMode();
    void AdjustZoom(float distanceMultiplier);
    void ChangeLayer(int layerDelta);
    void MoveSelectionOnGrid(int gridX, int gridZ);
    void UpdateSceneInteraction();
    void ChangeEditLayer(int layerDelta);
    void ToggleVerticalView();
    void SetFixedView(const glm::vec3& viewDirection);
    const glm::vec3& GetViewDirection() const;

private:
    UGCEditCommandController& mEditCommandController;
    UGCEditLayerController& mEditLayerController;
    UGCEditorViewController& mViewController;
    UGCSceneInteractionController& mSceneInteractionController;
};
