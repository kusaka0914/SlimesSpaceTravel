#pragma once

class StageSelectionController;

// アクティブな矩形選択範囲をImGuiの前景へ描画する。選択状態は変更しない。
void DrawStageSelectionOverlay(const StageSelectionController& selectionController);
