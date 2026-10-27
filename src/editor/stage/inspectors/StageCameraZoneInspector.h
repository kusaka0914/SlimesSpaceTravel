#pragma once

class CameraZone;
class Game;
class Platform;

// Camera Zone固有の構図と切り替え条件を編集する。カメラ登録・保存要求時にtrueを返す。呼び出し側で共通Actor保存を行う。
bool DrawCameraZoneInspector(CameraZone& zone, Game& game);

// 足場への接地を条件にするカメラ設定。登録・有効切替・保存要求時にtrueを返す。
bool DrawPlatformCameraInspector(Platform& platform, Game& game);
