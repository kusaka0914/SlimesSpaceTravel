#pragma once

#include "gfx/debug/DebugPanel.h"

#include <array>
#include <string>
#include <vector>

class TutorialController;
struct TutorialDefinition;
class TutorialLibrary;
struct TutorialPage;

// チュートリアルの本文・進行条件・注視対象・動画配置を編集し、ゲーム内プレビューと保存を提供する。
class TutorialDebugPanel final : public DebugPanel {
public:
    explicit TutorialDebugPanel(DebugEditorContext& context);

    // 共有ライブラリを直接編集する。再読み込み時はプレビューを停止し、保存操作でYAMLへ書き戻す。
    void Draw() override;

private:
    void DrawTutorialList(TutorialController* controller, TutorialLibrary& library);
    void DrawTutorialEditor(TutorialController* controller, TutorialLibrary& library);
    void DrawPageEditor(TutorialController* controller, TutorialLibrary& library, TutorialDefinition& definition,
                        std::vector<TutorialPage>& pages, std::size_t pageIndex);
    void DrawFocusTargetPicker(TutorialPage& page);
    void DrawVideoEditor(TutorialController* controller, TutorialDefinition& definition,
                         std::vector<TutorialPage>& pages, TutorialPage& page, std::size_t pageIndex);
    void DrawVideoPlacementOverlay(TutorialLibrary& library);

    std::array<char, 128> mNewTutorialId = {"new_tutorial"};
    std::array<char, 128> mVideoAssetFilter = {};
    std::string mSelectedTutorialId;
    std::string mPlacementTutorialId;
    std::string mStatusMessage;
    int mPlacementPageIndex = -1;
    bool mIsResizingVideoPlacement = false;
    bool mIsEditingAssistPages = false;
    bool mPlacementUsesAssistPages = false;
};
