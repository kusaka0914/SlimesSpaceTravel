#pragma once

class Game;

// 初期化済みGameのサービスを借用し、独立したActorとBulletワールドで幾何・移動・重力を検証する。
void RunRuntimeGameplayTests(Game& game);
