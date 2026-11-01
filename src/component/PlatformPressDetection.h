#pragma once

class Platform;
class Player;

// スイッチ共通の接触判定。押下の猶予時間・成立状態・演出は各Componentが管理する。
namespace PlatformPressDetection {

// 生存・操作ロックなしの接地中Playerについて、足元の直近の面がこの床ならtrue。物理形状も同期する。
bool IsPlayerPressingPlatform(const Player& player, const Platform& platform);

// GameのPlayer順で最初の押下者への非所有参照を返す。床・Gameが未設定、または押下者なしならnullptr。
Player* FindPlayerPressingPlatform(const Platform* platform);

} // namespace PlatformPressDetection
