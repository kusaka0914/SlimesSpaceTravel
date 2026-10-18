#pragma once

#include <functional>

// 同じマウス操作で続けて確定した編集を、直前の履歴へまとめる。
enum class StageEditGrouping { NewOperation, ContinueOperation };
using StageEditOperation = std::function<bool()>;
using StageEditExecutor = std::function<bool(const StageEditOperation&, StageEditGrouping)>;
