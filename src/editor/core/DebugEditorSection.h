#pragma once

// 編集画面のメインタブ。並び順の数値をセッションのactiveSectionIndexとして保存する。
enum class DebugEditorSection {
    BasicInfo,
    Parameters,
    Particles,
    Sequences,
    Tutorials,
    Stage,
    UserInterface,
};
