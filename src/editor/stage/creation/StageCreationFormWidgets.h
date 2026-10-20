#pragma once

struct DebugEditorContext;

namespace StageCreationFormWidgets {

// 有効範囲へ選択添字を補正して惑星一覧を描画する。惑星がない場合は -1。
void DrawPlanetCombo(DebugEditorContext& context, const char* label, int& selectedPlanetIndex);

} // namespace StageCreationFormWidgets
