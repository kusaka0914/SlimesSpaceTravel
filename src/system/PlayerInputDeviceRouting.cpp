#include "system/PlayerInputDeviceRouting.h"

PlayerInputDevice ResolvePlayerInputDevice(
    int playerNum,
    const PlayerInputDeviceContext& context)
{
    if (playerNum < 1 || playerNum > 2) {
        return PlayerInputDevice::None;
    }

    if (!context.isTwoPlayerMode) {
        if (playerNum != context.controlledPlayerNum) {
            return PlayerInputDevice::None;
        }
        return context.hasControllerOne
            ? PlayerInputDevice::ControllerOne
            : PlayerInputDevice::PrimaryKeyboard;
    }

    if (playerNum == 1) {
        return context.hasControllerOne
            ? PlayerInputDevice::ControllerOne
            : PlayerInputDevice::PrimaryKeyboard;
    }

    if (context.hasControllerTwo) {
        return PlayerInputDevice::ControllerTwo;
    }
    if (context.hasControllerOne) {
        return PlayerInputDevice::PrimaryKeyboard;
    }
    return context.isDebugMode
        ? PlayerInputDevice::SecondaryKeyboard
        : PlayerInputDevice::None;
}

int ResolveControllerPlayerNum(PlayerInputDevice inputDevice)
{
    if (inputDevice == PlayerInputDevice::ControllerOne) {
        return 1;
    }
    if (inputDevice == PlayerInputDevice::ControllerTwo) {
        return 2;
    }
    return 0;
}
