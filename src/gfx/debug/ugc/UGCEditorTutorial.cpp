#include "gfx/debug/ugc/UGCEditorTutorial.h"

void UGCEditorTutorial::Start()
{
    mStep = UGCEditorTutorialStep::Welcome;
    mIsActive = true;
}

void UGCEditorTutorial::AdvanceTo(UGCEditorTutorialStep nextStep)
{
    if (mIsActive) {
        mStep = nextStep;
    }
}

void UGCEditorTutorial::AdvanceFromWelcome()
{
    if (mStep == UGCEditorTutorialStep::Welcome) {
        AdvanceTo(UGCEditorTutorialStep::AdjustView);
    }
}

void UGCEditorTutorial::RecordViewAdjustment()
{
    if (mStep == UGCEditorTutorialStep::AdjustView) {
        AdvanceTo(UGCEditorTutorialStep::RaiseLayer);
    }
}

void UGCEditorTutorial::RecordLayerChange(
    int layerDelta,
    bool hasSelection)
{
    if (mStep == UGCEditorTutorialStep::RaiseLayer &&
        layerDelta > 0 && !hasSelection) {
        AdvanceTo(UGCEditorTutorialStep::LowerLayer);
        return;
    }
    if (mStep == UGCEditorTutorialStep::LowerLayer &&
        layerDelta < 0 && !hasSelection) {
        AdvanceTo(UGCEditorTutorialStep::PlacePlatform);
        return;
    }
    if (mStep == UGCEditorTutorialStep::RaiseSelectedPlatform &&
        layerDelta > 0 && hasSelection) {
        AdvanceTo(UGCEditorTutorialStep::LowerSelectedPlatform);
        return;
    }
    if (mStep == UGCEditorTutorialStep::LowerSelectedPlatform &&
        layerDelta < 0 && hasSelection) {
        AdvanceTo(UGCEditorTutorialStep::UndoEdit);
    }
}

void UGCEditorTutorial::RecordPlacement(
    UGCPresetKind presetKind,
    int footprintSideLength)
{
    if (mStep == UGCEditorTutorialStep::PlacePlatform &&
        presetKind == UGCPresetKind::NormalPlatform) {
        AdvanceTo(UGCEditorTutorialStep::PlaceLargePlatform);
        return;
    }
    if (mStep == UGCEditorTutorialStep::PlaceLargePlatform &&
        presetKind == UGCPresetKind::NormalPlatform &&
        footprintSideLength > 1) {
        AdvanceTo(UGCEditorTutorialStep::MovePlatform);
        return;
    }
    if (mStep == UGCEditorTutorialStep::PlaceEnemy &&
        presetKind == UGCPresetKind::NormalEnemy) {
        AdvanceTo(UGCEditorTutorialStep::PlaceGoal);
        return;
    }
    if (mStep == UGCEditorTutorialStep::PlaceGoal &&
        presetKind == UGCPresetKind::GoalStar) {
        AdvanceTo(UGCEditorTutorialStep::StartPlaytest);
    }
}

void UGCEditorTutorial::RecordSelectionMove()
{
    if (mStep == UGCEditorTutorialStep::MovePlatform) {
        AdvanceTo(UGCEditorTutorialStep::RaiseSelectedPlatform);
    }
}

void UGCEditorTutorial::RecordUndo(bool wasRestored)
{
    if (wasRestored && mStep == UGCEditorTutorialStep::UndoEdit) {
        AdvanceTo(UGCEditorTutorialStep::EraseObject);
    }
}

void UGCEditorTutorial::RecordErase(bool wasErased)
{
    if (wasErased && mStep == UGCEditorTutorialStep::EraseObject) {
        AdvanceTo(UGCEditorTutorialStep::PlaceEnemy);
    }
}

void UGCEditorTutorial::RecordPlaytestStarted()
{
    if (mStep == UGCEditorTutorialStep::StartPlaytest) {
        AdvanceTo(UGCEditorTutorialStep::ReturnFromPlaytest);
    }
}

void UGCEditorTutorial::RecordReturnedFromPlaytest()
{
    if (mStep == UGCEditorTutorialStep::ReturnFromPlaytest) {
        AdvanceTo(UGCEditorTutorialStep::Complete);
    }
}

std::string UGCEditorTutorial::GetInstruction() const
{
    switch (mStep) {
    case UGCEditorTutorialStep::Welcome:
        return "操作練習用のステージです。\n作品には保存されないので、自由に試せます。";
    case UGCEditorTutorialStep::AdjustView:
        return "「近づく」か「遠ざかる」で画面の見え方を変えてください。";
    case UGCEditorTutorialStep::RaiseLayer:
        return "何も選択していない状態で「上のだん」を押してください。";
    case UGCEditorTutorialStep::LowerLayer:
        return "何も選択していない状態で「下のだん」を押し、元のだんへ戻ってください。";
    case UGCEditorTutorialStep::PlacePlatform:
        return "上の「足場」を選び、ゲーム画面に1マス置いてください。";
    case UGCEditorTutorialStep::PlaceLargePlatform:
        return "足場の「4マス」か「9マス」を選び、もう一つ置いてください。";
    case UGCEditorTutorialStep::MovePlatform:
        return "選択モードにして足場を選び、長押ししたまま別の場所へ動かしてください。";
    case UGCEditorTutorialStep::RaiseSelectedPlatform:
        return "足場を長押ししている間に「上のだん」操作を行い、足場ごと上げてください。";
    case UGCEditorTutorialStep::LowerSelectedPlatform:
        return "足場を長押ししている間に「下のだん」操作を行い、足場ごと元のだんへ戻してください。";
    case UGCEditorTutorialStep::UndoEdit:
        return "「1つ戻す」で、いまの移動を元に戻してください。";
    case UGCEditorTutorialStep::EraseObject:
        return "消しゴムを選び、置いた足場を1マス消してください。";
    case UGCEditorTutorialStep::PlaceEnemy:
        return "上の「敵」を選び、ゲーム画面に置いてください。";
    case UGCEditorTutorialStep::PlaceGoal:
        return "上の「ゴール」を選び、ゲーム画面に置いてください。";
    case UGCEditorTutorialStep::StartPlaytest:
        return "左下の「遊ぶ」でテストプレイを始めてください。";
    case UGCEditorTutorialStep::ReturnFromPlaytest:
        return "ESC（コントローラーは－）で作る画面へ戻ってください。";
    case UGCEditorTutorialStep::Complete:
        return "基本操作は完了です。通常のステージ作成を始められます。";
    }
    return {};
}

std::string UGCEditorTutorial::GetProgressText() const
{
    if (mStep == UGCEditorTutorialStep::Welcome) {
        return "操作練習";
    }
    if (mStep == UGCEditorTutorialStep::Complete) {
        return "操作練習 完了";
    }
    return "操作練習 " + std::to_string(GetCurrentActionNumber()) +
        "/" + std::to_string(GetActionCount());
}

int UGCEditorTutorial::GetCurrentActionNumber() const
{
    if (mStep == UGCEditorTutorialStep::Welcome) {
        return 0;
    }
    if (mStep == UGCEditorTutorialStep::Complete) {
        return GetActionCount();
    }
    return static_cast<int>(mStep);
}

int UGCEditorTutorial::GetActionCount() const
{
    return static_cast<int>(UGCEditorTutorialStep::Complete) - 1;
}

float UGCEditorTutorial::GetProgressRatio() const
{
    return static_cast<float>(GetCurrentActionNumber()) /
        static_cast<float>(GetActionCount());
}

bool UGCEditorTutorial::ShouldHighlightPreset(UGCPresetKind presetKind) const
{
    if (mStep == UGCEditorTutorialStep::PlacePlatform ||
        mStep == UGCEditorTutorialStep::PlaceLargePlatform) {
        return presetKind == UGCPresetKind::NormalPlatform;
    }
    if (mStep == UGCEditorTutorialStep::PlaceEnemy) {
        return presetKind == UGCPresetKind::NormalEnemy;
    }
    if (mStep == UGCEditorTutorialStep::PlaceGoal) {
        return presetKind == UGCPresetKind::GoalStar;
    }
    return false;
}

bool UGCEditorTutorial::ShouldHighlightZoom() const
{
    return mStep == UGCEditorTutorialStep::AdjustView;
}

bool UGCEditorTutorial::ShouldHighlightLayerUp() const
{
    return mStep == UGCEditorTutorialStep::RaiseLayer ||
        mStep == UGCEditorTutorialStep::RaiseSelectedPlatform;
}

bool UGCEditorTutorial::ShouldHighlightLayerDown() const
{
    return mStep == UGCEditorTutorialStep::LowerLayer ||
        mStep == UGCEditorTutorialStep::LowerSelectedPlatform;
}

bool UGCEditorTutorial::ShouldHighlightSelection() const
{
    return mStep == UGCEditorTutorialStep::MovePlatform ||
        mStep == UGCEditorTutorialStep::RaiseSelectedPlatform ||
        mStep == UGCEditorTutorialStep::LowerSelectedPlatform;
}

bool UGCEditorTutorial::ShouldHighlightFootprintOptions() const
{
    return mStep == UGCEditorTutorialStep::PlaceLargePlatform;
}

bool UGCEditorTutorial::ShouldHighlightUndo() const
{
    return mStep == UGCEditorTutorialStep::UndoEdit;
}

bool UGCEditorTutorial::ShouldHighlightEraser() const
{
    return mStep == UGCEditorTutorialStep::EraseObject;
}

bool UGCEditorTutorial::ShouldHighlightPlaytest() const
{
    return mStep == UGCEditorTutorialStep::StartPlaytest;
}
