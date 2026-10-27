#pragma once

class CameraZone;
class Game;

// Camera Zone固有の構図と切り替え条件を編集する。カメラ登録・保存要求時にtrueを返す。呼び出し側で共通Actor保存を行う。
bool DrawCameraZoneInspector(CameraZone& zone, Game& game);
