#pragma once

class Platform;
class Player;

// スイッチ共通の接触判定。押下の猶予時間・成立状態・演出は各Componentが管理する。
namespace PlatformPressDetection {

// 有効なPlayerが床に支持され、接地中または上向き速度が閾値以下ならtrue。必要に応じて物理形状を同期する。
bool IsPlayerPressingPlatform(const Player& player, const Platform& platform);

// GameのPlayer順で最初の押下者への非所有参照を返す。床・Gameが未設定、または押下者なしならnullptr。
Player* FindPlayerPressingPlatform(const Platform* platform);

} // namespace PlatformPressDetection
