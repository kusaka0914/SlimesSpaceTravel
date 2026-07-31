#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <string>

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
        TutorialLibrary& library,
        TutorialDefinition& definition,
        std::size_t pageIndex);
    void DrawFocusTargetPicker(TutorialPage& page);

private:
    std::array<char, 128> mNewTutorialId = {"new_tutorial"};
    std::string mSelectedTutorialId;
    std::string mStatusMessage;
};
