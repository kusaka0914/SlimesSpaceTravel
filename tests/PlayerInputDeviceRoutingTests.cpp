#include "TestSupport.h"

#include "system/PlayerInputDeviceRouting.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {

void ExpectInputDevice(
    PlayerInputDevice expected,
    PlayerInputDevice actual,
    const std::string& expression)
{
    ExpectEqual(
        static_cast<int>(expected),
        static_cast<int>(actual),
        expression);
}

void TwoControllersAreAssignedToDifferentPlayers()
{
    const PlayerInputDeviceContext context{
        .isTwoPlayerMode = true,
        .hasControllerOne = true,
        .hasControllerTwo = true,
    };

    ExpectInputDevice(
        PlayerInputDevice::ControllerOne,
        ResolvePlayerInputDevice(1, context),
        "player one input device");
    ExpectInputDevice(
        PlayerInputDevice::ControllerTwo,
        ResolvePlayerInputDevice(2, context),
        "player two input device");
}

void OneControllerAndKeyboardAreAssignedToDifferentPlayers()
{
    const PlayerInputDeviceContext context{
        .isTwoPlayerMode = true,
        .hasControllerOne = true,
    };

    ExpectInputDevice(
        PlayerInputDevice::ControllerOne,
        ResolvePlayerInputDevice(1, context),
        "player one controller assignment");
    ExpectInputDevice(
        PlayerInputDevice::PrimaryKeyboard,
        ResolvePlayerInputDevice(2, context),
        "player two keyboard assignment");
}

void DebugKeyboardOnlyModeUsesSeparateKeySets()
{
    const PlayerInputDeviceContext context{
        .isTwoPlayerMode = true,
        .isDebugMode = true,
    };

    ExpectInputDevice(
        PlayerInputDevice::PrimaryKeyboard,
        ResolvePlayerInputDevice(1, context),
        "player one debug keyboard assignment");
    ExpectInputDevice(
        PlayerInputDevice::SecondaryKeyboard,
        ResolvePlayerInputDevice(2, context),
        "player two debug keyboard assignment");
}

void ControllerDisconnectDoesNotShareKeyboardBetweenPlayers()
{
    const PlayerInputDeviceContext context{
        .isTwoPlayerMode = true,
        .isDebugMode = false,
    };

    ExpectInputDevice(
        PlayerInputDevice::PrimaryKeyboard,
        ResolvePlayerInputDevice(1, context),
        "player one fallback input");
    ExpectInputDevice(
        PlayerInputDevice::None,
        ResolvePlayerInputDevice(2, context),
        "player two disconnected input");
}

void SinglePlayerControllerFollowsControlledSplitPlayer()
{
    const PlayerInputDeviceContext context{
        .hasControllerOne = true,
        .controlledPlayerNum = 2,
    };

    ExpectInputDevice(
        PlayerInputDevice::None,
        ResolvePlayerInputDevice(1, context),
        "inactive split player input");
    ExpectInputDevice(
        PlayerInputDevice::ControllerOne,
        ResolvePlayerInputDevice(2, context),
        "controlled split player input");
}

}

void RegisterPlayerInputDeviceRoutingTests(
    std::vector<std::pair<std::string, std::function<void()>>>& tests)
{
    tests.emplace_back(
        "PlayerInputDeviceRouting.TwoControllersAreAssignedToDifferentPlayers",
        TwoControllersAreAssignedToDifferentPlayers);
    tests.emplace_back(
        "PlayerInputDeviceRouting.OneControllerAndKeyboardAreAssignedToDifferentPlayers",
        OneControllerAndKeyboardAreAssignedToDifferentPlayers);
    tests.emplace_back(
        "PlayerInputDeviceRouting.DebugKeyboardOnlyModeUsesSeparateKeySets",
        DebugKeyboardOnlyModeUsesSeparateKeySets);
    tests.emplace_back(
        "PlayerInputDeviceRouting.ControllerDisconnectDoesNotShareKeyboardBetweenPlayers",
        ControllerDisconnectDoesNotShareKeyboardBetweenPlayers);
    tests.emplace_back(
        "PlayerInputDeviceRouting.SinglePlayerControllerFollowsControlledSplitPlayer",
        SinglePlayerControllerFollowsControlledSplitPlayer);
}
