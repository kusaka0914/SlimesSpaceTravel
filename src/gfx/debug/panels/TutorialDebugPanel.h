#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <string>
#include <vector>

class TutorialController;
struct TutorialDefinition;
class TutorialLibrary;
struct TutorialPage;

class TutorialDebugPanel final : public DebugPanel {
public:
    explicit TutorialDebugPanel(DebugEditorContext& context);

    void Draw() override;

private:
    void DrawTutorialList(
        TutorialController* controller,
        TutorialLibrary& library);
    void DrawTutorialEditor(
        TutorialController* controller,
        TutorialLibrary& library);
    void DrawPageEditor(
        TutorialController* controller,
        TutorialLibrary& library,
        TutorialDefinition& definition,
        std::size_t pageIndex);
    void DrawFocusTargetPicker(TutorialPage& page);
    void DrawVideoEditor(
        TutorialController* controller,
        TutorialDefinition& definition,
        TutorialPage& page,
        std::size_t pageIndex);
    void DrawVideoPlacementOverlay(TutorialLibrary& library);

private:
    std::array<char, 128> mNewTutorialId = {"new_tutorial"};
    std::array<char, 128> mVideoAssetFilter = {};
    std::string mSelectedTutorialId;
    std::string mPlacementTutorialId;
    std::string mStatusMessage;
    int mPlacementPageIndex = -1;
    bool mIsResizingVideoPlacement = false;
};
