#pragma once

class Game;

// 初期化済みGameのサービスを借用し、独立したActorとBulletワールドで幾何・移動・重力を検証する。
void RunRuntimeGameplayTests(Game& game);

// 操作スタイルの選択を伴うため、他の入力・シーン検証が終わった後に呼ぶ。保存先はRuntimeテスト用に隔離する。
void RunRuntimeCombatTests(Game& game);
