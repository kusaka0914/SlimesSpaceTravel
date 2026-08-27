#pragma once

#include "gfx/debug/DebugEditorContext.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorModelThumbnailRenderer.h"
#include "gfx/debug/panels/AssetBrowserPanel.h"
#include "gfx/debug/panels/CameraDebugPanel.h"
#include "gfx/debug/panels/EndingRollDebugPanel.h"
#include "gfx/debug/panels/ParameterDebugPanel.h"
#include "gfx/debug/panels/ParticleEffectDebugPanel.h"
#include "gfx/debug/panels/PerformanceDebugPanel.h"
#include "gfx/debug/panels/SequenceDebugPanel.h"
#include "gfx/debug/panels/StorybookDebugPanel.h"
#include "gfx/debug/panels/StageAddActorPanel.h"
#include "gfx/debug/panels/StageDeleteActorPanel.h"
#include "gfx/debug/panels/StageEditorPanel.h"
#include "gfx/debug/panels/StagePlacementPanel.h"
#include "gfx/debug/panels/StagePlanetPanel.h"
#include "gfx/debug/panels/TutorialDebugPanel.h"
#include "gfx/debug/panels/UIDebugPanel.h"
#include "gfx/debug/stage/StageEditCommandController.h"
#include "gfx/debug/stage/StageActorYamlWriter.h"
#include "gfx/debug/stage/StageGizmoController.h"
#include "gfx/debug/stage/StageSelectionController.h"
#include "gfx/debug/ugc/UGCWorkPanel.h"

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Game;
class UIRenderer;
struct EditorSessionState;

class DebugUIRenderer {
public:
    DebugUIRenderer(Game* game, UIRenderer* uiRenderer);

    void Draw(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawUGCEditor(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawUGCWorkBrowser();
    bool CompleteUGCVerification(const std::string& workFileName);
    void HandleUGCUndo();
    void HandleUGCRedo();
    void HandleUGCEraserToggle();
    void HandleUGCSelectionMode();
    void OpenUGCEditorMenu();
    void HandleUGCZoom(float distanceMultiplier);
    void HandleUGCLayerChange(int layerDelta);
    void HandleUGCSelectionGridMove(int gridX, int gridZ);

    bool SaveEditorSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    bool RestoreEditorSession(
        const std::string& filePath,
        std::string& outErrorMessage);
    void SetBuildRestartStatus(
        const std::string& message,
        bool isError);

private:
    enum class EditorSection {
        BasicInfo,
        Parameters,
        Particles,
        Sequences,
        Tutorials,
        Stage,
        UserInterface,
    };

    enum class UGCSwitchConnectionAction {
        Connect,
        Disconnect,
    };

    void DrawBasicInfoTab();
    void DrawSequenceEditorTab();
    void DrawStarCollectionEditor();
    void DrawDockedToolPanel(EditorSection section);
    void DrawDockedAssetBrowser(EditorSection section);
    void DrawGameViewport(
        EditorSection section,
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight);
    void DrawGameViewportToolbar(
        const ImVec2& toolbarMin,
        float toolbarWidth,
        bool showGizmoTranslationSpace);
    void ResolveResizableLayout(EditorSection section);
    void DrawLayoutResizeHandles(EditorSection section);
    const char* ResolveToolPanelTitle(EditorSection section) const;
    EditorSessionState CaptureEditorSessionState() const;
    void ApplyEditorSessionState(const EditorSessionState& sessionState);
    void AlignFreeCameraUpToSelectedActor();
    void DrawBuildRestartControls();
    void DrawUGCViewport(
        unsigned int gameViewTexture,
        int gameViewWidth,
        int gameViewHeight,
        const ImVec2& viewportMin,
        const ImVec2& viewportMax);
    void DrawUGCDebugEditorOverlay();
    void RegisterUGCUIEditorElements();
    void DrawUGCSwitchConnectionLines();
    void DrawUGCUnconnectedSwitchWarnings();
    void DrawUGCTransformControls();
    void ToggleUGCVerticalView();
    void SetUGCFixedView(const glm::vec3& viewDirection);
    void AdjustUGCViewDistance(float distanceMultiplier);
    void DrawUGCGridOverlay();
    void DrawUGCStackBadges();
    void DrawUGCPlacementPreview();
    void DrawUGCPreviewOverlay();
    void DrawUGCPreviewLayerGuides(
        const ImVec2& previewMin,
        const ImVec2& previewMax,
        ImDrawList* drawList);
    void DrawUGCWorkManagement();
    void StartUGCVerification();
    void UpdateUGCSelectionDrag();
    bool TryIntersectUGCDragPlane(
        const glm::vec3& rayFrom,
        const glm::vec3& rayTo,
        glm::vec3& outIntersection) const;
    void DrawUGCLayerControls();
    void ChangeUGCEditLayer(int layerDelta);
    void SyncUGCEditLayerToPickedActor();

private:
    EditorAssetCatalog mAssetCatalog;
    DebugEditorContext mContext;
    UGCWorkPanel mUGCWorkPanel;
    std::unique_ptr<EditorModelThumbnailRenderer> mUGCModelThumbnailRenderer;

    PerformanceDebugPanel mPerformancePanel;
    CameraDebugPanel mCameraPanel;
    UIDebugPanel mUIPanel;
    ParameterDebugPanel mParameterPanel;
    ParticleEffectDebugPanel mParticleEffectPanel;
    SequenceDebugPanel mSequencePanel;
    EndingRollDebugPanel mEndingRollPanel;
    StorybookDebugPanel mStorybookPanel;
    TutorialDebugPanel mTutorialPanel;
    AssetBrowserPanel mAssetBrowserPanel;

    StageAddActorPanel mStageAddActorPanel;
    StagePlanetPanel mStagePlanetPanel;
    StageActorYamlWriter mStageActorYamlWriter;

    StageSelectionController mSelectionController;
    StagePlacementPanel mStagePlacementPanel;
    StageEditCommandController mEditCommandController;
    StageDeleteActorPanel mStageDeleteActorPanel;
    StageEditorPanel mStageEditorPanel;
    StageGizmoController mGizmoController;

    EditorSection mActiveSection = EditorSection::BasicInfo;
    int mSelectedSequenceEditorMenu = 0;
    bool mShouldSelectRestoredSection = false;
    std::string mBuildRestartStatus;
    bool mIsBuildRestartStatusError = false;
    bool mIsUGCEraserMode = false;
    bool mShouldOpenUGCEditorMenu = false;
    std::optional<UGCPresetKind> mActiveUGCPresetKind;
    std::optional<UGCPresetKind> mUGCPresetBeforeEraser;
    glm::vec3 mUGCViewDirection{0.0f, 1.0f, 0.0f};
    std::string mUGCStatus;
    int mUGCEditLayer = 0;
    int mUGCPlatformFootprintSideLength = 1;
    std::optional<StageActorRef> mUGCConnectionSwitchRef;
    UGCSwitchConnectionAction mUGCSwitchConnectionAction =
        UGCSwitchConnectionAction::Connect;
    bool mIsUGCSelectionDragging = false;
    bool mIsUGCMovingPlatformDestinationDrag = false;
    bool mHasUGCSelectionDragMoved = false;
    glm::vec3 mUGCSelectionDragPlanePoint{0.0f};
    glm::vec3 mUGCSelectionDragPlaneNormal{0.0f, 1.0f, 0.0f};
    glm::vec3 mUGCSelectionDragOffset{0.0f};
    glm::vec3 mUGCSelectionDragInitialCenter{0.0f};
    glm::vec3 mUGCSelectionDragAppliedDelta{0.0f};
    glm::vec3 mUGCSelectionDragSavedDelta{0.0f};
    std::vector<StageActorRef> mUGCSelectionDragActorRefs;
    float mUGCPreviewWidth = 420.0f;
    float mUGCPreviewResizeStartWidth = 420.0f;
    bool mHasInitializedUGCPreviewWidth = false;
};
