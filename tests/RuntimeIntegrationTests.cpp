#include "TestFileSupport.h"
#include "actor/Actor.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/StageActorYamlWriter.h"
#include "editor/stage/StageEditCommandController.h"
#include "editor/stage/StageSelectionController.h"
#include "editor/stage/StageYamlRepository.h"
#include "editor/stage/UGCPlatformCellService.h"
#include "system/InputSystem.h"
#include "system/UserDataPaths.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraProjection.h"
#include "system/camera/PlayerCamera.h"
#include "system/camera/PlayerCameraSettings.h"
#include "world/Stage.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>

namespace {

struct IsolatedDirectory {
    IsolatedDirectory()
    {
        ExpectTrue(std::filesystem::create_directory(UserDataPaths::FindRootDirectory()),
                   "exclusive test data directory");
    }
    ~IsolatedDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(UserDataPaths::FindRootDirectory(), error);
    }
};

struct Fixture {
    IsolatedDirectory directory;
    Game game;
    Fixture()
    {
        // GameWindowは可視性のヒントを上書きしない。通常起動の設定は変えずテストだけ非表示にする。
        ExpectTrue(glfwInit() == GLFW_TRUE, "GLFW display available");
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        if (!game.Initialize(false, true)) {
            game.Shutdown();
            throw TestFailure("Game initialization failed");
        }
    }
    ~Fixture() { game.Shutdown(); }
};

StageSelectionScene SelectionScene(Game& game)
{
    return {game.GetCurrentStage(),    game.GetPhysicsSystem(),         game.GetCameraSystem(),
            game.GetWindow(),          game.GetCurrentStageYamlPath(),  game.GetIsUGCMode(),
            game.GetIsPlayer2Joined(), game.GetIsUGCOrthographicView(), game.GetUGCOrthographicHalfHeight(),
            game.GetUGCGridSize()};
}

StageActorInstance FindPlatform(Game& game)
{
    for (const auto& instance : StageActorQuery::CollectAllActorInstances(game.GetCurrentStage())) {
        if (instance.ref.type == StageActorType::Platform && instance.actor) {
            return instance;
        }
    }
    throw TestFailure("Expected a platform in test stage");
}

void EditingPersistsRuntimeUndoAndRedo(Game& game, DebugEditorContext& context, StageSelectionController& selection,
                                       StageEditCommandController& commands)
{
    const auto path = UserDataPaths::FindRootDirectory() / "test.yaml";
    std::filesystem::copy_file("../assets/data/stage/test.yaml", path);
    ExpectTrue(game.LoadStageForScene(1, path.string()), "load isolated stage");
    const std::string original = ReadTestFile(path);
    StageActorYamlWriter writer(context);
    auto target = FindPlatform(game);
    const auto originalScale = target.actor->GetScale();
    const auto editedScale = originalScale * 1.2f;
    target.actor->SetScale(editedScale);
    target.actor->CaptureEditorAuthoredScale();
    selection.SetSingleSelection(target.actor, target.ref);
    ExpectTrue(writer.SaveStage([&](const std::string& before) { commands.RecordSavedEdit(before); }),
               "save actual actor");
    ExpectTrue(target.actor->FindEditorAuthoredTransform() == nullptr, "clear saved authoring mark");
    const auto edited = ReadTestFile(path);
    ExpectTrue(original != edited, "persist edited YAML");
    ExpectTrue(commands.RestoreUndo(), "Undo with real runtime reload");
    ExpectEqual(original, ReadTestFile(path), "Undo original bytes");
    ExpectTrue(glm::length(FindPlatform(game).actor->GetScale() - originalScale) < 0.0001f, "Undo runtime scale");
    ExpectTrue(commands.RestoreRedo(), "Redo with real runtime reload");
    ExpectEqual(edited, ReadTestFile(path), "Redo edited bytes");
    ExpectTrue(glm::length(FindPlatform(game).actor->GetScale() - editedScale) < 0.0001f, "Redo runtime scale");

    // OS権限に頼らず、保存先の型を不正にして全OSで保存失敗を再現する。
    target = FindPlatform(game);
    target.actor->SetScale(editedScale * 1.1f);
    target.actor->CaptureEditorAuthoredScale();
    const auto retained = path.parent_path() / "retained.yaml";
    std::filesystem::rename(path, retained);
    std::filesystem::create_directory(path);
    ExpectFalse(writer.SaveStage([&](const std::string& before) { commands.RecordSavedEdit(before); }), "failed save");
    ExpectTrue(target.actor->FindEditorAuthoredTransform() != nullptr, "failure retains authoring mark");
    ExpectFalse(writer.GetSaveError().empty(), "failure exposed for retry");
    std::filesystem::remove(path);
    std::filesystem::rename(retained, path);
    ExpectEqual(edited, ReadTestFile(path), "old bytes retained");
    ExpectTrue(writer.RetryFailedSaves(), "retry actual save");
    ExpectTrue(target.actor->FindEditorAuthoredTransform() == nullptr, "retry clears mark");
    ExpectTrue(commands.RestoreUndo(), "retry recorded exactly one edit");
    ExpectEqual(edited, ReadTestFile(path), "Undo retry returns preceding edit");
    ExpectTrue(commands.RestoreUndo(), "first edit still in history");
    ExpectEqual(original, ReadTestFile(path), "Undo first edit returns original");
    ExpectFalse(commands.RestoreUndo(), "failed save did not add extra history");

    const auto other = path.parent_path() / "other.yaml";
    WriteTestFile(other, original);
    ExpectTrue(game.LoadStageForScene(1, other.string()), "switch stage");
    ExpectFalse(commands.RestoreRedo(), "history cannot cross stage boundary");
    ExpectEqual(original, ReadTestFile(other), "other file untouched");
    Actor* retainedActor = FindPlatform(game).actor;
    WriteTestFile(other, "planets: [");
    ExpectFalse(game.ReloadCurrentStage(), "actual parser failure propagates through Game");
    ExpectTrue(FindPlatform(game).actor == retainedActor, "failed load preserves previous Actor");
    WriteTestFile(other, original);
    ExpectTrue(game.ReloadCurrentStage(), "valid stage reloads after failure");
    std::cout << "[PASS] Runtime.StageSaveFailureRetryUndoRedoAndStageBoundary\n";
}

void UGCUsesTheSameTransaction(Game& game, DebugEditorContext& context, StageEditCommandController& commands)
{
    ExpectTrue(game.StartUGCMode(), "start isolated UGC");
    commands.SynchronizeStage();
    UGCPlatformCellService cells(context);
    const auto path = UserDataPaths::ResolveUGCWorkingStageFile();
    const std::string original = ReadTestFile(path);
    ExpectTrue(commands.ExecuteEdit([&]() { return cells.AddCells(0, {{8, 0, 0}}, 1.0f, 1, "normal"); }),
               "create generated cell");
    const std::string created = ReadTestFile(path);
    ExpectTrue(created != original, "cell persisted");
    auto target = FindPlatform(game);
    YAML::Node config;
    ExpectTrue(StageYamlRepository::LoadCurrentStage(context, config), "load actual cell config");
    ExpectTrue(cells.WriteTranslatedCells(config, {target.ref}, glm::vec3(0)) == UGCCellEditResult::Unchanged,
               "zero delta no-op");
    ExpectTrue(commands.ExecuteEdit([&]() {
        if (cells.WriteTranslatedCells(config, {target.ref}, glm::vec3(0, 1, 0)) != UGCCellEditResult::Changed) {
            return false;
        }
        return StageYamlRepository::SaveCurrentStage(context, config) && game.ReloadCurrentStage();
    }),
               "move generated cell and reload");
    ExpectTrue(commands.RestoreUndo(), "Undo cell translation");
    ExpectEqual(created, ReadTestFile(path), "Undo translation bytes");
    ExpectTrue(commands.RestoreUndo(), "Undo creation");
    ExpectEqual(original, ReadTestFile(path), "Undo creation bytes");
    std::cout << "[PASS] Runtime.UGCCreateMoveUndo\n";
}

void CameraManualYawRemainsIndependent(Game& game)
{
    ExpectFalse(game.GetPlayers().empty(), "runtime player loaded");
    Player* player = game.GetPlayers().front();
    CameraCollisionResolver collision(nullptr);
    PlayerCamera camera(collision);
    PlayerCameraSettings settings;
    player->UpdateActor(0.0f);
    camera.SnapToPlayer(player, 0);
    camera.GetView(player, 0, 10.0f, 0.3f, 1.0f);
    const auto initial = camera.GetCameraPos(0);
    // yawは先にPlayerの移動基準へ反映され、その後Cameraが追従する実フレームの順序を再現する。
    player->SetCameraYaw(0.4f);
    player->UpdateActor(0.0f);
    camera.Update({player}, {0.4f}, settings, 1.0f / 60.0f, true);
    camera.GetView(player, 0, 10.0f, 0.3f, 1.0f);
    const auto manual = camera.GetCameraPos(0);
    ExpectTrue(glm::length(manual - initial) > 0.1f, "manual yaw changes actual camera");
    camera.Update({player}, {0.0f}, settings, 1.0f / 60.0f, false);
    camera.GetView(player, 0, 10.0f, 0.3f, 1.0f);
    ExpectTrue(glm::length(camera.GetCameraPos(0) - manual) < 0.001f, "disabled assist preserves manual direction");
    const auto perspective = CalculateCameraProjection(CameraProjectionSettings{}, 16.0f / 9.0f, 60.0f);
    const auto ortho = CalculateCameraProjection(CameraProjectionSettings{true, 10.0f}, 16.0f / 9.0f, 60.0f);
    ExpectTrue(std::abs(perspective[3][3]) < 0.0001f && std::abs(ortho[3][3] - 1.0f) < 0.0001f,
               "projection modes preserved");
    std::cout << "[PASS] Runtime.ManualCameraAndProjection\n";
}

class VirtualController {
public:
    VirtualController()
    {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                                    SDL_CONTROLLER_BUTTON_MAX, 0);
        ExpectTrue(index >= 0, "attach isolated virtual controller");
        mInstanceId = SDL_JoystickGetDeviceInstanceID(index);
        mController = SDL_GameControllerOpen(index);
        if (!mController) {
            SDL_JoystickDetachVirtual(index);
            throw TestFailure("Failed to open virtual controller");
        }
    }
    ~VirtualController()
    {
        SDL_GameControllerClose(mController);
        // 実機の接続変更でSDLの添字が変わっても、自分が追加した機器だけを取り外す。
        for (int index = 0; index < SDL_NumJoysticks(); ++index) {
            if (SDL_JoystickGetDeviceInstanceID(index) == mInstanceId) {
                SDL_JoystickDetachVirtual(index);
                break;
            }
        }
    }
    SDL_GameController* Get() const { return mController; }
    void SetInput(Sint16 horizontal, Uint8 confirm)
    {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(mController);
        ExpectEqual(0, SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, horizontal), "set virtual axis");
        ExpectEqual(0, SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_A, confirm), "set virtual button");
        SDL_JoystickUpdate();
        SDL_GameControllerUpdate();
    }

private:
    SDL_JoystickID mInstanceId = -1;
    SDL_GameController* mController = nullptr;
};

void InputSnapshotUsesOnlyProvidedDevices(Game& game)
{
    VirtualController controller;
    InputSystem input;
    controller.SetInput(20000, 1);
    input.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
    ExpectTrue(input.HasControllerInput(1), "provided controller connected");
    ExpectFalse(input.HasControllerInput(2), "unprovided controller absent regardless of real devices");
    ExpectEqual(20000, input.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX), "axis captured");
    ExpectTrue(input.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A), "confirm captured");
    ExpectTrue(input.IsMovementInputPressed(PlayerInputDevice::ControllerOne), "movement routed to captured device");
    ExpectFalse(input.IsMovementInputPressed(PlayerInputDevice::None), "unassigned input does not move");
    controller.SetInput(0, 0);
    ExpectEqual(20000, input.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX), "same-frame query preserves snapshot");
    input.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
    ExpectEqual(0, input.GetControllerAxis(1, SDL_CONTROLLER_AXIS_LEFTX), "next frame observes release");
    ExpectFalse(input.IsControllerButtonPressed(1, SDL_CONTROLLER_BUTTON_A), "next frame button released");
    input.CaptureFrameInput(*game.GetWindow(), {nullptr, nullptr});
    ExpectFalse(input.HasControllerInput(1), "disconnect clears connected flag");
    ExpectFalse(input.IsMovementInputPressed(PlayerInputDevice::ControllerOne), "disconnect clears movement");
    ExpectEqual(0, input.GetControllerAxis(0, SDL_CONTROLLER_AXIS_LEFTX), "invalid player number ignored");
    ExpectEqual(0, input.GetControllerAxis(1, -1), "invalid axis ignored");
    std::cout << "[PASS] Runtime.InputSnapshotAndVirtualController\n";
}

} // namespace

int main()
{
    try {
        {
            Fixture fixture;
            Game& game = fixture.game;
            DebugEditorContext context{&game};
            StageSelectionController selection([&]() { return SelectionScene(game); }, context.gameViewport,
                                               context.planetMoveMode);
            StageEditCommandController commands(context, selection);
            EditingPersistsRuntimeUndoAndRedo(game, context, selection, commands);
            CameraManualYawRemainsIndependent(game);
            UGCUsesTheSameTransaction(game, context, commands);
            InputSnapshotUsesOnlyProvidedDevices(game);
            ExpectTrue(ImGui::GetCurrentContext() != nullptr, "Editor owns initialized ImGui");
            game.Shutdown();
            ExpectTrue(ImGui::GetCurrentContext() == nullptr, "Editor context released before window shutdown");
            game.Shutdown();
        }
        {
            Fixture restarted;
            ExpectTrue(ImGui::GetCurrentContext() != nullptr, "fresh Game initializes after previous shutdown");
            restarted.game.Shutdown();
            ExpectTrue(ImGui::GetCurrentContext() == nullptr, "second Editor context also released");
        }
        std::cout << "[PASS] Runtime.EditorLifetimeAndRepeatedStartup\n";
        std::cout << "Runtime integration tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] Runtime integration: " << error.what() << '\n';
        return 1;
    }
}
