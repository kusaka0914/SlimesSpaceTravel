#pragma once

enum class PlayerInputDevice {
    None,
    PrimaryKeyboard,
    SecondaryKeyboard,
    ControllerOne,
    ControllerTwo,
};

struct PlayerInputDeviceContext {
    bool isTwoPlayerMode = false;
    bool isDebugMode = false;
    bool hasControllerOne = false;
    bool hasControllerTwo = false;
    int controlledPlayerNum = 1;
};

// 1始まりのPlayer番号を、参加状態と接続状況に応じた入力機器へ割り当てる。対象外はNone。
PlayerInputDevice ResolvePlayerInputDevice(int playerNum, const PlayerInputDeviceContext& context);

// コントローラーの1始まりの番号を返す。キーボード・Noneでは0を返す。
int ResolveControllerPlayerNum(PlayerInputDevice inputDevice);
