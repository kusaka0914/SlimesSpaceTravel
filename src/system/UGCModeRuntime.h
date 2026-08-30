#pragma once

#include <functional>
#include <string>

struct CameraPose;

class UGCModeRuntime {
public:
    virtual ~UGCModeRuntime() = default;

    virtual bool IsTitleScene() const = 0;
    virtual void SetDebugEditorShowing(bool isShowing) = 0;
    virtual void SetFreeCameraMode(bool isEnabled) = 0;
    virtual void ReloadCurrentStage() = 0;
    virtual bool LoadDebugStage(
        int stageNumber,
        const std::string& yamlPath) = 0;
    virtual void ClosePauseMenu() = 0;
    virtual void SetUGCOrthographicHalfHeight(float halfHeight) = 0;
    virtual void StartPlayingScene() = 0;
    virtual void StartUGCStageClearPresentation() = 0;
    virtual void SetDebugCameraPose(const CameraPose& pose) = 0;
    virtual bool RequestSceneFadeAction(
        const std::function<void()>& fadeAction) = 0;
    virtual void NotifyUGCTutorialReturnedFromPlaytest() = 0;
    virtual void CompleteUGCVerification(
        const std::string& workFileName) = 0;
    virtual bool HasProgressFlag(const std::string& progressId) const = 0;
    virtual void MarkProgressFlag(const std::string& progressId) = 0;
    virtual void EnterTitleAtFadeMidpoint() = 0;
    virtual void TryChangeBGM() = 0;
};
