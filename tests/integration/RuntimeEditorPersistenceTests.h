#pragma once

class Game;

// 独立したActorを現在のStageへ一時登録し、実際のWriterで保存スキーマと編集状態の契約を検証する。
void RunRuntimeEditorPersistenceTests(Game& game);
