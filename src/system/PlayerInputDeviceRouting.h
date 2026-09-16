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

PlayerInputDevice ResolvePlayerInputDevice(
    int playerNum,
    const PlayerInputDeviceContext& context);

int ResolveControllerPlayerNum(PlayerInputDevice inputDevice);
