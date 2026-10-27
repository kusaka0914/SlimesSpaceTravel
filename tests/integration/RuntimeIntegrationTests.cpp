#include "RuntimeEditorPersistenceTests.h"
#include "RuntimeFrameLoopTests.h"
#include "RuntimeGameplayTests.h"
#include "RuntimeReliabilityTests.h"
#include "actor/Actor.h"
#include "actor/CameraZone.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "editor/session/EditorBuildRestartService.h"
#include "editor/stage/StageActorQuery.h"
#include "editor/stage/history/StageEditCommandController.h"
#include "editor/stage/persistence/StageActorYamlWriter.h"
#include "editor/stage/persistence/StageYamlRepository.h"
#include "editor/stage/selection/StageSelectionController.h"
#include "editor/stage/ugc/UGCPlatformCellService.h"
#include "gfx/Renderer3D.h"
#include "gfx/UIRenderer.h"
#include "gfx/particle/ParticleRenderer.h"
#include "gfx/performance/FramePerformanceTracker.h"
#include "gfx/render3d/RenderViewportController.h"
#include "support/RuntimeStageSupport.h"
#include "support/TestFileSupport.h"
#include "system/CameraSystem.h"
#include "system/InputSystem.h"
#include "system/PhysicsSystem.h"
#include "system/UserDataPaths.h"
#include "system/camera/CameraCollisionResolver.h"
#include "system/camera/CameraLateralOrbit.h"
#include "system/camera/CameraProjection.h"
#include "system/camera/PlayerCamera.h"
#include "system/camera/PlayerCameraSettings.h"
#include "system/camera/zones/CameraZoneController.h"
#include "system/sequence/SequenceSystem.h"
#include "world/GameWorld.h"
#include "world/Stage.h"

#include "imgui.h"
#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <utility>

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
    explicit Fixture(bool areDebugToolsEnabled = false, bool isVisible = false)
    {
        // Editorの保存先は実行ファイル基準なので、通常ビルドの設定と共有していないことを確認する。
        if (areDebugToolsEnabled) {
            const char* token = std::getenv("SLIMES_RUNTIME_TEST_TOKEN");
            ExpectTrue(token != nullptr, "launch debug Runtime checks through tools/run_runtime_checks.py");
            std::filesystem::path session;
            std::string error;
            ExpectTrue(EditorBuildRestartService{}.ResolvePersistentDebugSessionFilePath(session, error),
                       "resolve isolated Editor session");
            ExpectEqual(std::string(token), ReadTestFile(session.parent_path() / ".runtime-test-token"),
                        "Editor sessions stay beside the private test executable");
        }
        // GameWindowは可視性のヒントを上書きしない。通常起動の設定は変えずテストだけ非表示にする。
        ExpectTrue(glfwInit() == GLFW_TRUE, "GLFW display available");
        glfwWindowHint(GLFW_VISIBLE, isVisible ? GLFW_TRUE : GLFW_FALSE);
        if (!game.Initialize(areDebugToolsEnabled, true)) {
            game.Shutdown();
            throw TestFailure("Game initialization failed");
        }
        ImGui::GetIO().IniFilename = nullptr;
    }
    ~Fixture() = default;
};

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
    // 手動yawはCameraが消費し、描画した視線をPlayerの移動基準へ返す。
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

void RendererDrawsStageAndBorrowedParticles(Game& game)
{
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "initialization leaves no GL error");
    game.GetCameraSystem()->Update(1.0f / 60.0f);
    ExpectFalse(game.GetCameraSystem()->GetViews().empty(), "stage camera is drawable");
    game.GetRenderer3D()->RenderGameplayShadowMap();
    game.GetRenderer3D()->Draw();
    glFinish();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "stage and shadow drawing succeed");

    Particle particle;
    particle.texturePath = "spark_dot.png";
    particle.lifetime = 1.0f;
    particle.startSize = 0.25f;
    std::array<Particle, 2> particles{particle, particle};
    particles[1].blendMode = ParticleBlendMode::Alpha;
    const auto view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0), glm::vec3(0, 1, 0));
    const auto projection = CalculateCameraProjection(CameraProjectionSettings{}, 1.0f, 60.0f);
    glUseProgram(0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    {
        ParticleRenderer renderer;
        renderer.Draw(view, projection, particles);
        GLint program = -1;
        GLboolean depthWrite = GL_FALSE;
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);
        ExpectEqual(0, program, "particle drawing restores program");
        ExpectFalse(glIsEnabled(GL_BLEND) == GL_TRUE, "particle drawing restores blend state");
        ExpectTrue(depthWrite == GL_TRUE, "particle drawing restores depth write");
    }
    glFinish();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "particle drawing and release succeed");
    std::cout << "[PASS] Runtime.StageShadowAndBorrowedParticleRendering\n";
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

    void SetCombatInput(bool weak, bool strong, bool recover)
    {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(mController);
        ExpectEqual(0, SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_Y, weak), "set weak button");
        ExpectEqual(0, SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_X, strong), "set strong button");
        ExpectEqual(0, SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_LEFTSHOULDER, recover),
                    "set recovery button");
        SDL_JoystickUpdate();
        SDL_GameControllerUpdate();
    }

    void SetCameraInput(Sint16 yaw, Sint16 pitch)
    {
        SDL_Joystick* joystick = SDL_GameControllerGetJoystick(mController);
        ExpectEqual(0, SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTX, yaw), "set camera yaw");
        ExpectEqual(0, SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_RIGHTY, pitch), "set camera pitch");
        SDL_JoystickUpdate();
        SDL_GameControllerUpdate();
    }

private:
    SDL_JoystickID mInstanceId = -1;
    SDL_GameController* mController = nullptr;
};

struct CombatControlsFixture {
    Player player;
    PlayerInput input;
    PlayerMovement movement;
    PlayerGrounding grounding;
    PlayerBoatRide boatRide;
    PlayerCombat combat;
    PlayerJewelGauge jewels;
    PlayerStatus status;
    PlayerRespawn respawn;
    PlayerStateMachine state;
    PlayerInputContext context{
        .devices = {.isTwoPlayerMode = true, .hasControllerOne = true, .hasControllerTwo = true}};

    CombatControlsFixture(Game& game, InputSystem& source, int playerNumber)
        : player(&game),
          input(source),
          movement(*game.GetPhysicsSystem(), *game.GetMathUtils()),
          grounding(*game.GetPhysicsSystem()),
          combat(MakeTestCombatDependencies(game))
    {
        player.SetPlayerNum(playerNumber);
        movement.SetPlayerNum(playerNumber);
        player.Land({1000, 1000, 1000});
        status.ConfigureHp(10);
        status.SetHp(5);
        combat.SetAttackHitDelay(0.1f);
    }

    void Step(float seconds)
    {
        input.ProcessActor(movement, context);
        state.Update(player, input, movement, grounding, boatRide, combat, jewels, status, respawn, seconds);
    }
};

void LongPressCombatControlsUseIndependentDevices(Game& game)
{
    const auto previousStyle = game.GetPlayerControlStyle();
    struct RestoreStyle {
        Game& game;
        PlayerControlStyle style;
        ~RestoreStyle() { game.SetPlayerControlStyle(style); }
    } restore{game, previousStyle};
    game.SetPlayerControlStyle(PlayerControlStyle::Standard);
    VirtualController firstController;
    VirtualController secondController;
    InputSystem source;
    const auto capture = [&]() {
        source.CaptureFrameInput(*game.GetWindow(), {firstController.Get(), secondController.Get()});
    };

    {
        CombatControlsFixture first(game, source, 1);
        CombatControlsFixture second(game, source, 2);
        firstController.SetCombatInput(true, false, false);
        secondController.SetCombatInput(false, true, false);
        capture();
        first.Step(0.1f);
        second.Step(0.1f);
        ExpectTrue(first.combat.HasPendingAttackHit(), "Y starts an attack on the press frame");
        ExpectTrue(second.state.GetActionState() == PlayerActionState::Idle, "X waits for release");
        secondController.SetCombatInput(false, false, false);
        capture();
        first.Step(0.1f);
        second.Step(0.1f);
        ExpectTrue(second.state.GetActionState() == PlayerActionState::Attacking, "P2 X release starts strong attack");
        for (int frame = 0; frame < 6; ++frame) {
            first.Step(0.1f);
        }
        ExpectTrue(first.combat.IsContinuousAttacking(), "held Y transitions after resolving weak attack");
        ExpectEqual(0, first.jewels.GetCount(), "continuous attack consumes exactly two jewels");
        ExpectEqual(2, second.jewels.GetCount(), "P1 hold does not consume P2 jewels");
        first.jewels.SetCount(6);
        for (int frame = 0; frame < 30; ++frame) {
            first.Step(0.1f);
        }
        ExpectEqual(6, first.jewels.GetCount(), "held Y cannot restart continuous attack");
    }

    for (const PlayerControlStyle style : {PlayerControlStyle::Standard, PlayerControlStyle::Assist}) {
        game.SetPlayerControlStyle(style);
        CombatControlsFixture fixture(game, source, 1);
        firstController.SetCombatInput(false, false, false);
        capture();
        fixture.Step(0.01f);
        firstController.SetCombatInput(false, true, false);
        capture();
        fixture.Step(0.7f);
        ExpectTrue(fixture.combat.IsSpecialCharging(), "long X starts charging in either style");
        ExpectNear(5.0f, fixture.status.GetHp(), 0.0f, "long X does not heal");
        firstController.SetCombatInput(false, true, true);
        capture();
        for (int frame = 0; frame < 40; ++frame) {
            fixture.Step(0.1f);
        }
        ExpectTrue(fixture.combat.GetCanSpecialAttack(), "original hold does not fire completed charge");
        ExpectEqual(0, fixture.jewels.GetCount(), "charge retains original two-jewel cost");
        ExpectNear(5.0f, fixture.status.GetHp(), 0.0f, "L cannot heal during charge");
        const auto before = fixture.combat.GetResolvedAttackSequence();
        firstController.SetCombatInput(false, false, false);
        capture();
        fixture.Step(0.01f);
        ExpectEqual(before + 1, fixture.combat.GetResolvedAttackSequence(), "release fires charged attack once");
        fixture.jewels.SetCount(2);
        fixture.Step(0.01f);
        ExpectNear(5.0f, fixture.status.GetHp(), 0.0f, "charge release does not trigger assist recovery");
        ExpectEqual(2, fixture.jewels.GetCount(), "release consumes no further jewels");
        ExpectEqual(before + 1, fixture.combat.GetResolvedAttackSequence(), "no repeated fire while released");
    }

    for (const PlayerControlStyle style : {PlayerControlStyle::Standard, PlayerControlStyle::Assist}) {
        game.SetPlayerControlStyle(style);
        for (int heldFrames : {5, 15, 25}) {
            CombatControlsFixture fixture(game, source, 1);
            firstController.SetCombatInput(false, true, false);
            capture();
            fixture.Step(0.7f);
            ExpectNear(0.75f, fixture.combat.GetInputMovementSpeedMultiplier(), 0.0f, "charge movement is 75 percent");
            for (int frame = 0; frame < heldFrames; ++frame) {
                fixture.Step(0.1f);
            }
            const auto before = fixture.combat.GetResolvedAttackSequence();
            firstController.SetCombatInput(false, false, false);
            capture();
            fixture.Step(0.01f);
            ExpectFalse(fixture.combat.IsSpecialCharging() || fixture.combat.GetCanSpecialAttack(),
                        "early release cancels");
            ExpectEqual(2, fixture.jewels.GetCount(), "early release refunds only consumed jewels");
            ExpectEqual(before, fixture.combat.GetResolvedAttackSequence(), "early release does not fire");
            ExpectNear(5.0f, fixture.status.GetHp(), 0.0f, "cancellation cannot trigger assist recovery");
            ExpectNear(1.0f, fixture.combat.GetInputMovementSpeedMultiplier(), 0.0f, "cancel restores normal movement");
        }
        CombatControlsFixture interrupted(game, source, 1);
        interrupted.combat.StartSpecialAttackCharging();
        ExpectTrue(interrupted.combat.ConsumeSpecialAttackJewel(interrupted.jewels), "charge consumes a jewel");
        interrupted.combat.CancelSpecialAttack();
        ExpectEqual(1, interrupted.jewels.GetCount(), "damage interruption does not refund");
        interrupted.combat.CancelSpecialAttackAndRefund(interrupted.jewels);
        ExpectEqual(1, interrupted.jewels.GetCount(), "interrupted charge cannot be refunded later");
    }

    game.SetPlayerControlStyle(PlayerControlStyle::Assist);
    {
        CombatControlsFixture fixture(game, source, 1);
        firstController.SetCombatInput(false, true, false);
        capture();
        fixture.Step(0.2f);
        firstController.SetCombatInput(false, false, false);
        capture();
        fixture.Step(0.01f);
        ExpectNear(6.0f, fixture.status.GetHp(), 0.0f, "assist short X heals on release");
        ExpectEqual(1, fixture.jewels.GetCount(), "assist recovery costs one jewel");
        firstController.SetCombatInput(false, false, true);
        capture();
        fixture.Step(0.01f);
        ExpectNear(7.0f, fixture.status.GetHp(), 0.0f, "L uses same recovery independently");
    }

    for (bool weak : {false, true}) {
        CombatControlsFixture fixture(game, source, 1);
        fixture.jewels.SetCount(1);
        firstController.SetCombatInput(weak, !weak, false);
        capture();
        fixture.Step(0.7f);
        ExpectFalse(fixture.combat.IsSpecialCharging() || fixture.combat.IsContinuousAttacking(),
                    "insufficient jewels reject special attack");
        ExpectEqual(1, fixture.jewels.GetCount(), "rejected special attack consumes no jewels");
        firstController.SetCombatInput(false, false, false);
        capture();
        fixture.Step(0.01f);
        ExpectNear(5.0f, fixture.status.GetHp(), 0.0f, "rejected long press does not become recovery");
    }
    std::cout << "[PASS] Runtime.LongPressCombatControlsAndCosts\n";
}

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
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    movement.SetPlayerNum(1);
    PlayerInput playerInput(input);
    PlayerInputContext context{.devices = {.hasControllerOne = true}};
    playerInput.ProcessActor(movement, context);
    ExpectTrue(playerInput.GetJumpPressed(), "injected device context reads controller jump");
    ExpectTrue(playerInput.GetMoveLeft() > 0.5f, "injected device context reads movement");
    context.isWaitingForTutorialJump = true;
    playerInput.ProcessActor(movement, context);
    ExpectNear(0.0f, playerInput.GetMoveLeft(), 0.0f, "tutorial restriction blocks movement");
    ExpectTrue(playerInput.GetJumpPressed(), "tutorial restriction retains requested jump");
    context.isNonControlledSoloPlayer = true;
    playerInput.ProcessActor(movement, context);
    ExpectFalse(playerInput.GetJumpPressed(), "noncontrolled snapshot clears held jump");
    context.isNonControlledSoloPlayer = false;
    context.isWaitingForTutorialJump = false;
    playerInput.SetInputAvailableTimer(1.0f);
    context.isWaitingForTutorialJump = true;
    playerInput.ProcessActor(movement, context);
    ExpectFalse(playerInput.GetJumpPressed(), "input lock prevents tutorial jump until available");

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

void TwoPlayerViewportAndEmptyTargetsRemainSafe(Game& game)
{
    game.TryCreatePlayer2();
    ExpectTrue(game.GetIsPlayer2Joined(), "debug runtime joins second player");
    ExpectTrue(game.GetPlayers().size() >= 2 && game.GetPlayers()[1]->GetIsActive(), "second player is active");
    game.GetCameraSystem()->Update(1.0f / 60.0f);
    game.GetRenderer3D()->Draw();
    glFinish();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "two viewports render without GL errors");
    ExpectEqual(2, game.GetFramePerformanceMetrics().renderedViewportCount, "both viewport times reach the tracker");
    game.ReturnToSinglePlayer();
    ExpectFalse(game.GetIsPlayer2Joined(), "return to single player");
    game.GetRenderer3D()->Draw();
    glFinish();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "single viewport works after two-player return");

    FramePerformanceTracker performance;
    RenderViewportController viewports(*game.GetCameraSystem(), performance, game.GetRenderer3D());
    SceneRenderState scene;
    scene.isPlayer2Joined = true;
    viewports.DrawGameScreen(640, 0, scene);
    viewports.DrawGameScreen(0, 480, scene);
    ExpectEqual(0, performance.GetMetrics().renderedViewportCount,
                "empty targets do not render or record invalid projections");
    viewports.Shutdown();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "empty viewport targets leave no GL error");
    std::cout << "[PASS] Runtime.TwoPlayerViewportAndEmptyTargets\n";
}

void UGCSceneUsesCurrentBorrowedPreviewState(Game& game)
{
    ExpectTrue(game.GetIsUGCMode(), "UGC editor is active");
    game.SetUGCPlatformPlacementPreview(glm::vec3(8, 0, 0));
    game.SetUGCMovingPlatformPathPreview(glm::vec3(0, 0, 0), glm::vec3(3, 0, 0));
    const glm::vec3 cameraPosition(12, 20, 20);
    const auto view = glm::lookAt(cameraPosition, glm::vec3(0), glm::vec3(0, 1, 0));
    const auto projection = CalculateCameraProjection(CameraProjectionSettings{true, 20}, 4.0f / 3.0f, 60.0f);
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(),
                "UGC editing leaves no GL error before rendering");
    glViewport(0, 0, 640, 480);
    game.GetRenderer3D()->RenderShadowMap(glm::vec3(0));
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "UGC shadow pass");
    game.GetRenderer3D()->DrawScene(view, projection, cameraPosition, UGCSceneLayerRenderMode::HighlightEditingLayer,
                                    game.GetUGCPreviewEditLayer());
    glFinish();
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "UGC layer, ghost and moving path render");
    GLfloat originalWidth = 0.0f;
    glGetFloatv(GL_LINE_WIDTH, &originalWidth);
    game.GetRenderer3D()->DrawAttackRangeVertices({glm::vec3(0), glm::vec3(1, 0, 0)}, GL_LINES, glm::vec4(1), 2.0f);
    GLfloat restoredWidth = 0.0f;
    glGetFloatv(GL_LINE_WIDTH, &restoredWidth);
    ExpectNear(originalWidth, restoredWidth, 0.0f, "explicit line width is restored after drawing");
    ExpectEqual(static_cast<unsigned int>(GL_NO_ERROR), glGetError(), "line width respects renderer capabilities");
    game.SetUGCPlatformPlacementPreview(std::nullopt);
    game.SetUGCMovingPlatformPathPreview(std::nullopt, std::nullopt);
    std::cout << "[PASS] Runtime.UGCSceneAndPlacementPreview\n";
}

void CustomUIKeepsOrderAndFitsViewport(Game& game)
{
    UIRenderer& renderer = *game.GetUIRenderer();
    auto& elements = renderer.GetUILoadSystem()->GetCustomElements();
    struct RestoreElements {
        std::vector<UILoadSystem::CustomElement>& elements;
        std::vector<UILoadSystem::CustomElement> original;
        ~RestoreElements() { elements = std::move(original); }
    } restore{elements, std::move(elements)};

    UILoadSystem::CustomElement first;
    first.screen = "test_layout";
    first.id = "first";
    first.type = UILoadSystem::CustomElementType::Panel;
    first.visibleByDefault = true;
    first.xRatio = 0.1f;
    first.yRatio = 2.0f;
    first.widthRatio = 0.2f;
    first.heightRatio = 0.1f;
    first.zOrder = 1;
    auto second = first;
    second.id = "second";
    second.yRatio = 2.4f;
    auto behind = first;
    behind.id = "behind";
    behind.type = UILoadSystem::CustomElementType::Text;
    behind.text = "First line\nSecond line";
    behind.yRatio = 2.8f;
    behind.zOrder = 0;
    auto hidden = first;
    hidden.id = "hidden";
    hidden.visibleByDefault = false;
    elements = {first, second, behind, hidden};

    ExpectEqual(static_cast<GLenum>(GL_NO_ERROR), glGetError(), "previous rendering leaves clean GL state");
    renderer.DrawGameContent();
    ExpectEqual(static_cast<GLenum>(GL_NO_ERROR), glGetError(), "custom UI draws without GL errors");
    ExpectTrue(glIsEnabled(GL_DEPTH_TEST) == GL_TRUE, "UI restores depth test");
    GLboolean writesDepth = GL_FALSE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &writesDepth);
    ExpectTrue(writesDepth == GL_TRUE, "UI restores depth writes");

    std::vector<std::string> order;
    for (const auto& rendered : renderer.GetRenderedUIElements()) {
        if (rendered.source != UIRenderer::RenderedUIElementSource::Custom || rendered.screen != first.screen) {
            continue;
        }
        order.push_back(rendered.id);
        const auto& transform = rendered.transform;
        const float top = transform.center.y - transform.size.y * 0.5f;
        const float bottom = transform.center.y + transform.size.y * 0.5f;
        ExpectTrue(top >= 11.0f && bottom <= renderer.GetFbHeight() - 11.0f,
                   "oversized authored content fits inside viewport padding");
        ExpectTrue(std::isfinite(transform.center.x) && transform.size.x > 0 && transform.size.y > 0,
                   "finite nonempty UI bounds");
    }
    ExpectEqual(std::size_t(3), order.size(), "only visible synthetic UI elements are recorded");
    ExpectEqual(std::string("behind"), order[0], "lower zOrder first");
    ExpectEqual(std::string("first"), order[1], "equal zOrder retains authored first element");
    ExpectEqual(std::string("second"), order[2], "equal zOrder retains authored second element");
    std::cout << "[PASS] Runtime.CustomUILayoutOrderAndViewportBounds\n";
}

void CamerasUseExplicitContextWithoutGame()
{
    DebugCamera debug;
    CameraPose authored;
    authored.position = {1, 2, 3};
    authored.target = {1, 2, 13};
    authored.fieldOfViewDegrees = 200.0f;
    debug.SetPose(authored, true);
    ExpectTrue(debug.GetPose(true).target == authored.target, "UGC preserves authored target distance");
    ExpectNear(1.0f, glm::length(debug.GetPose(false).target - authored.position), 0.0001f,
               "free camera returns unit look direction");
    ExpectNear(120.0f, debug.GetFieldOfViewDegrees(), 0.0f, "field of view remains clamped");
    authored.target = authored.position;
    authored.up = {0, 0, 0};
    debug.SetPose(authored, false);
    const auto view = debug.GetView();
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            ExpectTrue(std::isfinite(view[column][row]), "degenerate pose keeps a finite camera basis");
        }
    }
    CameraCollisionResolver collision(nullptr);
    FocusCamera focus(collision);
    ExpectTrue(focus.GetOpeningViews(OpeningCameraShot::None).empty(), "no opening shot selected");
    ExpectEqual(std::size_t(1), focus.GetOpeningViews(OpeningCameraShot::Mother).size(), "mother shot");
    ExpectEqual(std::size_t(1), focus.GetOpeningViews(OpeningCameraShot::Doctor).size(), "doctor shot");
    std::cout << "[PASS] Runtime.CameraExplicitContextAndDegeneratePose\n";
}

void NormalCameraSeparatesPositionAndSight(Game& game)
{
    Player player(&game);
    CameraCollisionResolver collision(nullptr);
    PlayerCamera camera(collision);
    PlayerCameraSettings settings;
    settings.backwardFacingLookAheadDistance = 0.0f;
    const glm::vec3 origin(0.0f);
    constexpr float stepSeconds = 1.0f / 60.0f;
    const auto reset = [&](const glm::vec3& position, const glm::vec3& up) {
        player.SetPos(position);
        player.SetUpVec(up);
        player.SetOnGround(true);
        player.SetCameraForwardDirection({0, 0, 1}, up);
        camera.SnapToPlayer(&player, 0);
        camera.GetView(&player, 0, settings.distance, glm::radians(settings.pitchDegrees), settings.targetHeight);
    };
    const auto step = [&](const glm::vec3& position, const glm::vec3& up) {
        player.SetPos(position);
        player.SetUpVec(up);
        camera.Update({&player}, {0.0f}, settings, stepSeconds, true);
        return camera.GetView(&player, 0, settings.distance, glm::radians(settings.pitchDegrees),
                              settings.targetHeight);
    };

    for (float side : {-1.0f, 1.0f}) {
        reset(origin, {0, 1, 0});
        const auto initialPosition = camera.GetCameraPos(0);
        const auto initialDirection = player.GetForwardVec();
        for (int frame = 0; frame < 600; ++frame) {
            const auto lateral = glm::normalize(glm::cross(player.GetUpVec(), player.GetForwardVec()));
            step(player.GetPos() + lateral * (side * 0.03f), player.GetUpVec());
            ExpectTrue(glm::distance(initialPosition, camera.GetCameraPos(0)) < 0.0001f,
                       "sustained lateral motion keeps the camera position fixed");
            const auto rawBack = camera.GetCameraPos(0) - player.GetPos();
            const auto radial = glm::normalize(rawBack - player.GetUpVec() * glm::dot(rawBack, player.GetUpVec()));
            ExpectTrue(glm::dot(radial, player.GetForwardVec()) > 0.99999f,
                       "movement basis uses the current player position instead of the delayed look target");
        }
        ExpectTrue(glm::dot(initialDirection, player.GetForwardVec()) < 0.95f,
                   "sight tracking changes the camera-relative movement basis without a back-align timer");
        const auto stopped = player.GetPos();
        for (int frame = 0; frame < 120; ++frame) {
            step(stopped, player.GetUpVec());
        }
        ExpectTrue(glm::distance(initialPosition, camera.GetCameraPos(0)) < 0.0001f,
                   "finishing sight interpolation does not translate the camera");
    }

    for (float forward : {-1.0f, 1.0f}) {
        reset(origin, {0, 1, 0});
        const auto before = camera.GetCameraPos(0);
        step(player.GetPos() + glm::vec3(0.04f, 0, forward * 0.04f), player.GetUpVec());
        const auto displacement = camera.GetCameraPos(0) - before;
        ExpectTrue(forward * displacement.z > 0.0f, "forward and backward motion translate the camera");
        ExpectNear(0.0f, displacement.x, 0.0001f, "diagonal displacement excludes lateral camera translation");
    }

    reset(origin, {0, 1, 0});
    const auto beforeJump = camera.GetCameraPos(0);
    for (int jump = 0; jump < 3; ++jump) {
        player.SetOnGround(false);
        for (int frame = 1; frame <= 60; ++frame) {
            const float height = std::sin(glm::pi<float>() * frame / 60.0f) * 2.0f;
            step(origin + glm::vec3(0, height, 0), {0, 1, 0});
        }
        player.SetOnGround(true);
        for (int frame = 0; frame < 120; ++frame) {
            step(origin, {0, 1, 0});
        }
        ExpectTrue(glm::distance(camera.GetCameraPos(0), beforeJump) < 0.001f,
                   "repeated jumps do not accumulate camera height drift");
    }

    // 同じ大円を球面と楕円面で一周し、法線が上下反転する区間も通過する。
    for (float verticalRadius : {40.0f, 25.0f}) {
        reset(origin + glm::vec3(0, verticalRadius, 0), {0, 1, 0});
        const auto initial = camera.GetCameraPos(0);
        for (int frame = 1; frame <= 1260; ++frame) {
            const float angle = glm::two_pi<float>() * frame / 1260.0f;
            const glm::vec3 local(0, verticalRadius * std::cos(angle), -40.0f * std::sin(angle));
            const auto up =
                glm::normalize(glm::vec3(0, local.y / (verticalRadius * verticalRadius), local.z / 1600.0f));
            const auto view = step(origin + local, up);
            for (int column = 0; column < 4; ++column) {
                for (int row = 0; row < 4; ++row) {
                    ExpectTrue(std::isfinite(view[column][row]), "curved-surface view remains finite");
                }
            }
            ExpectTrue(glm::distance(camera.GetCameraPos(0), player.GetPos()) < 30.0f,
                       "longitudinal tracking follows the far side of the planet");
        }
        ExpectTrue(glm::distance(camera.GetCameraPos(0), initial) < 5.0f,
                   "a full longitudinal circuit returns the camera near its starting position");
    }

    reset(origin, {0, 1, 0});
    CameraPose zonePose;
    zonePose.position = origin + glm::vec3(7, 5, 0);
    zonePose.target = origin + glm::vec3(0, settings.targetHeight, 0);
    zonePose.up = {0, 1, 0};
    camera.AdoptZonePose(0, zonePose, player);
    for (int frame = 0; frame < 120; ++frame) {
        step(player.GetPos(), player.GetUpVec());
    }
    const auto restoredOffset = camera.GetCameraPos(0) - player.GetPos();
    ExpectNear(settings.distance, glm::length(restoredOffset), 0.001f, "zone exit restores normal distance");
    ExpectNear(glm::radians(settings.pitchDegrees),
               std::atan2(-restoredOffset.y, glm::length(glm::vec2(restoredOffset.x, restoredOffset.z))), 0.001f,
               "zone exit restores normal pitch");
    ExpectTrue(glm::dot(player.GetForwardVec(), glm::vec3(1, 0, 0)) > 0.99f,
               "normal framing restoration preserves zone azimuth instead of aligning behind the player");
    player.SetFacingForwardVec({0, 0, 1});
    camera.AlignBehindPlayer(&player, 0);
    for (int frame = 0; frame < 120; ++frame) {
        step(player.GetPos(), player.GetUpVec());
    }
    ExpectTrue(glm::dot(player.GetForwardVec(), -player.GetFacingForwardVec()) > 0.99f,
               "an explicit reset still aligns behind the player");
    std::cout << "[PASS] Runtime.NormalCameraPositionAndSightTracking\n";
}

void SphereCameraFollowsLateralMovement(Game& game)
{
    Planet sphere(&game);
    sphere.SetScale(glm::vec3(40));
    Planet ellipse(&game);
    ellipse.SetScale({40, 25, 40});
    Player player(&game);
    player.Actor::SetCurrentPlanet(&sphere);
    CameraCollisionResolver collision(nullptr);
    PlayerCamera camera(collision);
    PlayerCameraSettings settings;
    settings.backwardFacingLookAheadDistance = 0.0f;
    const auto render = [&]() {
        camera.Update({&player}, {0.0f}, settings, 1.0f / 60.0f, true);
        camera.GetView(&player, 0, settings.distance, glm::radians(settings.pitchDegrees), settings.targetHeight);
    };
    for (float side : {-1.0f, 1.0f}) {
        player.Actor::SetCurrentPlanet(&sphere);
        player.SetPos({0, 40, 0});
        player.SetUpVec({0, 1, 0});
        player.SetOnGround(true);
        player.SetCameraForwardDirection({0, 0, 1}, player.GetUpVec());
        camera.SnapToPlayer(&player, 0);
        render();
        const auto initial = camera.GetCameraPos(0);
        for (int frame = 0; frame < 180; ++frame) {
            player.SetPos(player.GetPos() + glm::vec3(side * 0.03f, 0, 0));
            render();
        }
        ExpectTrue(side * (camera.GetCameraPos(0).x - initial.x) > 5.0f,
                   "Sphere lateral movement translates the camera with the player");
        ExpectTrue(glm::dot(player.GetForwardVec(), glm::vec3(0, 0, 1)) > 0.9999f,
                   "Sphere lateral follow does not turn into a stationary-camera orbit");

        const auto beforeExit = camera.GetCameraPos(0);
        player.Actor::SetCurrentPlanet(&ellipse);
        render();
        ExpectTrue(glm::distance(beforeExit, camera.GetCameraPos(0)) < 0.0001f,
                   "leaving Sphere preserves the current camera position");
        for (int frame = 0; frame < 60; ++frame) {
            const auto lateral = glm::normalize(glm::cross(player.GetUpVec(), player.GetForwardVec()));
            player.SetPos(player.GetPos() + lateral * (side * 0.03f));
            render();
        }
        const auto fixedPosition = camera.GetCameraPos(0);
        ExpectTrue(glm::distance(beforeExit, fixedPosition) < 0.0001f,
                   "Ellipse retains stationary lateral camera tracking");

        player.Actor::SetCurrentPlanet(&sphere);
        render();
        ExpectTrue(glm::distance(fixedPosition, camera.GetCameraPos(0)) < 0.1f,
                   "entering Sphere begins a smooth positional transition");
        for (int frame = 0; frame < 120; ++frame) {
            render();
        }
        ExpectNear(settings.distance, glm::distance(player.GetPos(), camera.GetCameraPos(0)), 0.001f,
                   "Sphere transition restores the configured camera distance");
    }
    // 小さい球体では法線の変化が速く、描画用の姿勢補間を追従方向へ戻す誤差が顕著になる。
    for (float radius : {4.0f, 40.0f}) {
        sphere.SetScale(glm::vec3(radius));
        player.Actor::SetCurrentPlanet(&sphere);
        player.SetPos({0, radius, 0});
        player.SetUpVec({0, 1, 0});
        player.SetCameraForwardDirection({0, 0, 1}, player.GetUpVec());
        camera.SnapToPlayer(&player, 0);
        render();
        const auto initial = camera.GetCameraPos(0);
        for (int frame = 1; frame <= 600; ++frame) {
            const float angle = frame * 0.0025f;
            const glm::vec3 up(std::sin(angle), std::cos(angle), 0);
            player.SetPos(up * radius);
            player.SetUpVec(up);
            render();
            ExpectTrue(glm::dot(player.GetForwardVec(), glm::vec3(0, 0, 1)) > 0.9999f,
                       "curved Sphere lateral follow does not accumulate yaw from smoothed up vectors");
            const auto firstViewPosition = camera.GetCameraPos(0);
            camera.GetView(&player, 0, settings.distance, glm::radians(settings.pitchDegrees), settings.targetHeight);
            ExpectTrue(glm::distance(firstViewPosition, camera.GetCameraPos(0)) < 0.0001f,
                       "repeated view reads do not change Sphere orbit direction");
        }
        ExpectTrue(glm::distance(initial, camera.GetCameraPos(0)) > radius,
                   "the Sphere camera travels with the player around the surface");
    }
    std::cout << "[PASS] Runtime.SphereCameraLateralFollow\n";
}

void NormalCameraRestoresFramingAfterZoneExit(Game& game, bool sphereFollow)
{
    CameraZone zone(&game);
    Planet planet(&game);
    planet.SetScale(sphereFollow ? glm::vec3(40) : glm::vec3(40, 25, 40));
    planet.AddStageObject(&zone);
    auto* stage = game.GetCurrentStage();
    ExpectTrue(stage != nullptr, "zone exit test has a stage");
    ScopedTestStagePlanets planets(*stage, {&planet});
    Player player(&game);
    player.Actor::SetCurrentPlanet(&planet);
    player.SetPos({0, 0, 0});
    player.SetUpVec({0, 1, 0});
    player.SetOnGround(true);
    zone.SetStageSequenceName("cameraZones");
    zone.SetStageYamlIndex(0);
    zone.SetPos({0, 0, 0});
    auto& zoneSettings = zone.GetSettings();
    zoneSettings.mode = CameraZoneSettings::Mode::Fixed;
    zoneSettings.fixedTracksPlayer = false;
    zoneSettings.fixedPose = CameraPose{{20, 18, 0}, {0, 1.5f, 0}, {0, 1, 0}, 60};
    zoneSettings.enterSeconds = 0;
    zoneSettings.exitSeconds = 0.7f;
    PlayerCameraSettings settings;
    CameraCollisionResolver collision(nullptr);
    PlayerCamera camera(collision);
    CameraZoneController zones;
    std::uint64_t frame = 0;
    const auto render = [&]() {
        camera.Update({&player}, {0.0f}, settings, 1.0f / 60.0f, true);
        const auto view =
            camera.GetView(&player, 0, settings.distance, glm::radians(settings.pitchDegrees), settings.targetHeight);
        const auto inverse = glm::inverse(view);
        CameraPose normal{camera.GetCameraPos(0), camera.GetLookTarget(0), glm::vec3(inverse[1]),
                          settings.fieldOfViewDegrees};
        const auto pose =
            zones.Apply(player, 0, normal, stage, {}, collision, 16.0f / 9.0f, 1.0f / 60.0f, ++frame, true);
        if (zones.GetPose(0)) {
            camera.AdoptZonePose(0, pose, player);
            player.SetCameraForwardDirection(pose.position - pose.target, pose.up);
        }
        return pose;
    };
    const auto inside = render();
    ExpectTrue(zones.HasActiveZone(0), "the test begins inside the zone");
    ExpectTrue(glm::distance(inside.position, zoneSettings.fixedPose.position) < 0.0001f,
               "the zone overrides normal tracking for both planet shapes");
    player.SetPos({8, 0, 0});
    const auto firstExit = render();
    ExpectFalse(zones.HasActiveZone(0), "leaving the box releases zone selection");
    ExpectTrue(zones.GetPose(0).has_value(), "exit blends before releasing the output");
    ExpectTrue(glm::distance(firstExit.position, inside.position) < 1.0f, "exit does not snap to normal distance");
    CameraPose outside;
    for (int index = 0; index < 120; ++index) {
        outside = render();
    }
    ExpectFalse(zones.GetPose(0).has_value(), "the zone eventually releases its output");
    const auto offset = outside.position - player.GetPos();
    ExpectNear(settings.distance, glm::length(offset), 0.001f,
               "exit reaches normal distance despite repeated pose adoption");
    ExpectNear(glm::radians(settings.pitchDegrees), std::atan2(-offset.y, glm::length(glm::vec2(offset.x, offset.z))),
               0.001f, "exit reaches normal pitch");
    const auto settled = outside.position;
    player.SetPos(player.GetPos() + glm::vec3(0, 0, 0.1f));
    const float lateralTranslation = glm::distance(render().position, settled);
    if (sphereFollow) {
        ExpectTrue(lateralTranslation > 0.0001f, "Sphere resumes lateral position follow after zone exit");
    } else {
        ExpectTrue(lateralTranslation < 0.0001f, "Ellipse resumes fixed lateral tracking after zone exit");
    }
    std::cout << "[PASS] Runtime.NormalCameraRestoresZoneExitFraming sphere=" << sphereFollow << "\n";
}

void LateralWalkingKeepsNormalCameraRadius(Game& game)
{
    VirtualController controller;
    InputSystem inputSystem;
    PlayerInput input(inputSystem);
    Player player(&game);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    movement.SetMoveSpeed(6.0f);
    const PlayerInputContext context{
        .devices = {.hasControllerOne = true},
    };
    const glm::vec3 origin(1000, 1000, 1000);
    const auto cameraPosition = origin + glm::vec3(0, 6, 4);
    const float radius = glm::distance(origin, cameraPosition);
    for (int frameRate : {30, 60, 120}) {
        for (Sint16 horizontal : {Sint16(-32000), Sint16(32000)}) {
            for (float speedMultiplier : {1.0f, 0.75f}) {
                controller.SetInput(horizontal, 0);
                inputSystem.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
                input.ProcessActor(movement, context);
                ExpectTrue(std::abs(input.GetMoveLeft()) > 0.9f, "lateral movement input is held");
                ExpectNear(0.0f, input.GetMoveForward(), 0.0f, "the radius test has no forward input");
                player.SetPos(origin);
                player.SetUpVec({0, 1, 0});
                player.SetOnGround(true);
                movement.SetCameraForwardDirection({0, 0, 1}, {0, 1, 0});
                movement.SetNormalCameraPosition(cameraPosition);
                const float seconds = 1.0f / frameRate;
                for (int frame = 0; frame < frameRate * 10; ++frame) {
                    movement.UpdateCameraRelativeMovementDirections(player, input);
                    const auto before = player.GetPos();
                    const auto cameraBack = movement.GetForwardVec();
                    movement.MoveFromInput(player, input, seconds, speedMultiplier);
                    const auto follow = player.GetPos() - before - movement.GetCameraLateralCorrection();
                    ExpectNear(0.0f, glm::dot(follow, cameraBack), 0.001f,
                               "free lateral arc correction is excluded from forward camera tracking");
                    ExpectNear(radius, glm::distance(player.GetPos(), cameraPosition), 0.05f,
                               "ten seconds of lateral walking do not spiral away from the camera");
                }
                // Zoneからの方向指定では通常カメラ用の中間接線補正を解除する。
                player.SetPos(origin);
                movement.SetCameraForwardDirection({0, 0, 1}, {0, 1, 0});
                movement.UpdateCameraRelativeMovementDirections(player, input);
                movement.MoveFromInput(player, input, seconds, speedMultiplier);
                ExpectNear(origin.z, player.GetPos().z, 0.0001f,
                           "explicit camera direction keeps zone movement unchanged");
            }
        }
    }
    std::cout << "[PASS] Runtime.LateralWalkingPreservesNormalCameraRadius\n";
}

void LateralOrbitAssistPreservesDistanceAndTightensTurns(Game& game)
{
    VirtualController controller;
    InputSystem inputSystem;
    PlayerInput input(inputSystem);
    Player player(&game);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    movement.SetMoveSpeed(6.0f);
    const PlayerInputContext context{.devices = {.hasControllerOne = true}};
    const glm::vec3 origin(1000, 1000, 1000);
    const glm::vec3 up(0, 1, 0);
    const auto initialCamera = origin + glm::vec3(0, 6, 4);
    const float radius = glm::distance(origin, initialCamera);
    for (int frameRate : {30, 60, 120}) {
        for (Sint16 horizontal : {Sint16(-32000), Sint16(32000)}) {
            float unassistedYaw = 0.0f;
            for (float strength : {0.0f, 0.25f, 0.5f}) {
                controller.SetInput(horizontal, 0);
                inputSystem.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
                input.ProcessActor(movement, context);
                player.SetPos(origin);
                player.SetUpVec(up);
                player.SetOnGround(true);
                movement.SetCameraForwardDirection({0, 0, 1}, up);
                auto camera = initialCamera;
                movement.SetNormalCameraPosition(camera);
                const float seconds = 1.0f / frameRate;
                float totalYaw = 0.0f;
                for (int frame = 0; frame < frameRate * 2; ++frame) {
                    movement.UpdateCameraRelativeMovementDirections(player, input);
                    const auto beforeDirection = movement.GetForwardVec();
                    movement.MoveFromInput(player, input, seconds);
                    const auto inputDelta = movement.ConsumeCameraInputMovementDelta();
                    ExpectNear(0.0f, glm::length(movement.ConsumeCameraInputMovementDelta()), 0.0f,
                               "camera input displacement is consumed only once");
                    if (const auto rotation =
                            CameraLateralOrbit::CalculateRotation(camera, player.GetPos(), up, inputDelta, strength)) {
                        camera = player.GetPos() + *rotation * (camera - player.GetPos());
                    }
                    if (frame == 0 && strength > 0.0f) {
                        ExpectTrue(horizontal * (camera.x - initialCamera.x) < 0.0f,
                                   "the camera orbits opposite to the player's lateral displacement");
                    }
                    movement.SetCameraForwardDirection(camera - player.GetPos(), up);
                    movement.SetNormalCameraPosition(camera);
                    const auto afterDirection = movement.GetForwardVec();
                    totalYaw += std::atan2(glm::dot(glm::cross(beforeDirection, afterDirection), up),
                                           glm::dot(beforeDirection, afterDirection));
                    ExpectNear(radius, glm::distance(player.GetPos(), camera), 0.05f,
                               "orbit assistance preserves player-camera distance");
                    ExpectNear(6.0f, camera.y - player.GetPos().y, 0.0001f, "orbit assistance preserves camera height");
                    if (strength == 0.0f) {
                        ExpectTrue(glm::distance(initialCamera, camera) < 0.0001f,
                                   "zero orbit strength preserves the fixed camera position");
                    }
                }
                if (strength == 0.0f) {
                    unassistedYaw = std::abs(totalYaw);
                } else {
                    ExpectTrue(std::abs(totalYaw) > unassistedYaw * (1.0f + strength * 0.9f),
                               "higher orbit strength turns farther over the same walking distance");
                }
                controller.SetInput(0, 0);
                inputSystem.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
                input.ProcessActor(movement, context);
                movement.UpdateCameraRelativeMovementDirections(player, input);
                movement.MoveFromInput(player, input, seconds);
                ExpectFalse(CameraLateralOrbit::CalculateRotation(camera, player.GetPos(), up,
                                                                  movement.ConsumeCameraInputMovementDelta(), strength)
                                .has_value(),
                            "releasing lateral input stops additional orbit immediately");
            }
        }
    }
    ExpectFalse(CameraLateralOrbit::CalculateRotation({0, 6, 4}, {0, 0, 0}, up, {0, 0, 0.1f}, 0.5f).has_value(),
                "forward-only movement does not add lateral orbit");
    ExpectFalse(CameraLateralOrbit::CalculateRotation({0, 6, 0}, {0, 0, 0}, up, {0.1f, 0, 0}, 0.5f).has_value(),
                "an overhead camera does not produce unstable lateral orbit");
    std::cout << "[PASS] Runtime.LateralOrbitAssistPreservesDistance\n";
}

void LateralWallSlideRetainsForwardCameraMovement(Game& game)
{
    auto* world = game.GetPhysicsSystem()->GetBulletWorld();
    ExpectTrue(world != nullptr, "wall-slide camera test has a physics world");
    const glm::vec3 origin(1000, 1000, 1000);
    btBoxShape shape(btVector3(0.05f, 10, 10));
    struct Wall {
        btDiscreteDynamicsWorld& world;
        btCollisionObject object;
        Wall(btDiscreteDynamicsWorld& world, btCollisionShape& shape, const glm::vec3& origin)
            : world(world)
        {
            object.setCollisionShape(&shape);
            btTransform transform = btTransform::getIdentity();
            const auto rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 1, 0));
            transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
            transform.setOrigin(btVector3(origin.x + 1.2f, origin.y, origin.z - 1.2f));
            object.setWorldTransform(transform);
            world.addCollisionObject(&object, btBroadphaseProxy::DefaultFilter, btBroadphaseProxy::AllFilter);
        }
        ~Wall() { world.removeCollisionObject(&object); }
    } wall(*world, shape, origin);
    VirtualController controller;
    controller.SetInput(-32000, 0);
    InputSystem source;
    source.CaptureFrameInput(*game.GetWindow(), {controller.Get(), nullptr});
    PlayerInput input(source);
    PlayerMovement movement(*game.GetPhysicsSystem(), *game.GetMathUtils());
    input.ProcessActor(movement, PlayerInputContext{.devices = {.hasControllerOne = true}});
    ExpectTrue(std::abs(input.GetMoveLeft()) > 0.9f && input.GetMoveForward() == 0,
               "wall slide is driven by lateral input alone");
    Player player(&game);
    player.SetPos(origin);
    player.SetUpVec({0, 1, 0});
    player.SetOnGround(true);
    movement.SetMoveSpeed(6.0f);
    movement.SetCameraForwardDirection({0, 0, 1}, {0, 1, 0});
    movement.SetNormalCameraPosition(origin + glm::vec3(0, 6, 4));
    bool sawForwardSlide = false;
    for (int frame = 0; frame < 60; ++frame) {
        movement.UpdateCameraRelativeMovementDirections(player, input);
        const auto before = player.GetPos();
        const auto back = movement.GetForwardVec();
        movement.MoveFromInput(player, input, 1.0f / 60.0f);
        const auto follow = player.GetPos() - before - movement.ConsumeCameraLateralCorrection();
        sawForwardSlide = sawForwardSlide || std::abs(glm::dot(follow, back)) > 0.005f;
        ExpectTrue(glm::length(movement.ConsumeCameraLateralCorrection()) == 0,
                   "camera correction is consumed once, including when subsequent world updates are paused");
    }
    ExpectTrue(sawForwardSlide, "forward wall-slide displacement remains available to camera tracking");
    std::cout << "[PASS] Runtime.LateralWallSlideKeepsForwardCameraMovement\n";
}

void ScriptedPlayerMovementRestoresNormalCamera(Game& game)
{
    Player* player = game.GetMainPlayer();
    CameraSystem* camera = game.GetCameraSystem();
    ExpectTrue(player && camera, "scripted movement has a player and camera");
    Planet ellipse(&game);
    ellipse.SetScale({40, 10, 40});
    struct RestorePlayer {
        Player& player;
        CameraSystem& camera;
        Planet* planet;
        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 facing;
        bool onGround;
        PlayerCameraSettings settings;
        ~RestorePlayer()
        {
            player.Actor::SetCurrentPlanet(planet);
            player.SetPos(position);
            player.SetUpVec(up);
            player.SetFacingForwardVec(facing);
            player.SetOnGround(onGround);
            camera.SetPlayerCameraSettings(settings);
            camera.SnapBehindPlayer(player);
        }
    } restore{*player,
              *camera,
              player->GetCurrentPlanet(),
              player->GetPos(),
              player->GetUpVec(),
              player->GetFacingForwardVec(),
              player->GetOnGround(),
              camera->GetPlayerCameraSettings()};

    auto settings = restore.settings;
    settings.backwardFacingLookAheadDistance = 0.0f;
    camera->SetPlayerCameraSettings(settings);
    const glm::vec3 origin(1000, 1000, 1000);
    player->Actor::SetCurrentPlanet(&ellipse);
    player->SetPos(origin);
    player->SetUpVec({0, 1, 0});
    player->SetFacingForwardVec({0, 0, -1});
    player->SetOnGround(true);
    camera->SnapBehindPlayer(*player);
    camera->GetViews();

    SequenceSystem sequences(&game);
    constexpr const char* sequenceId = "runtime_player_arrival";
    ExpectTrue(sequences.GetLibrary().Create(sequenceId), "create in-memory arrival sequence");
    SequenceClip movement;
    movement.actor = {"players", 0};
    movement.fromPosition = origin;
    movement.toPosition = origin + glm::vec3(6, -6, 0);
    movement.duration = 1.0f;
    sequences.GetLibrary().FindMutable(sequenceId)->clips.push_back(movement);
    ExpectTrue(sequences.Play(sequenceId), "start scripted player arrival");
    const auto expectedPosition = [&]() {
        return player->GetPos() + (-player->GetFacingForwardVec() * std::cos(glm::radians(settings.pitchDegrees)) -
                                   player->GetUpVec() * std::sin(glm::radians(settings.pitchDegrees))) *
                                      settings.distance;
    };
    // 到着後だけでなく、降下中にも左右・高さの演出変位を追い、ロケットの真下へ視点を残さない。
    for (int step = 0; step < 2; ++step) {
        sequences.Update(0.25f);
        camera->Update(1.0f / 60.0f);
        ExpectFalse(camera->GetViews().empty(), "camera remains available during descent");
        ExpectTrue(glm::distance(camera->GetPlayerCameraPos(0), expectedPosition()) < 0.001f,
                   "scripted descent preserves normal distance and pitch before the sequence ends");
    }
    sequences.Update(0.5f);
    ExpectFalse(sequences.IsPlaying(), "arrival sequence finishes");
    ExpectTrue(glm::distance(player->GetPos(), movement.toPosition) < 0.0001f, "arrival reaches its destination");
    ExpectFalse(camera->GetViews().empty(), "normal camera resumes after arrival");

    const auto expected = expectedPosition();
    ExpectTrue(glm::distance(camera->GetPlayerCameraPos(0), expected) < 0.001f,
               "arrival discards the scripted offset and restores configured distance and pitch");
    camera->Update(1.0f / 60.0f);
    camera->GetViews();
    ExpectTrue(glm::distance(camera->GetPlayerCameraPos(0), expected) < 0.001f,
               "arrival framing remains stable on the following frame");
    ExpectNear(settings.distance, camera->GetPlayerCameraSettings().distance, 0.0f,
               "arrival preserves camera settings");
    std::cout << "[PASS] Runtime.ScriptedPlayerMovementRestoresNormalCamera\n";
}

void CameraControlsAreIndependentAndStopDuringEditorCapture(Game& game)
{
    Player* first = game.GetMainPlayer();
    ExpectTrue(first != nullptr, "camera test has a primary player");
    Player second(&game);
    second.SetPos(first->GetPos());
    second.SetUpVec(first->GetUpVec());
    second.Actor::SetCurrentPlanet(first->GetCurrentPlanet());
    GameWorld world;
    world.AddPlayer(first);
    world.AddPlayer(&second);
    InputSystem input;
    CameraControlState controls{
        .mainPlayer = first,
        .controlledPlayer = first,
        .isPlayer2Joined = true,
        .hasPrimaryController = true,
        .hasSecondController = true,
    };
    CameraSystem camera(CameraSystemDependencies{
        .world = world,
        .input = input,
        .scene = *game.GetSceneSystem(),
        .readControlState = [&]() { return controls; },
    });
    VirtualController primary;
    VirtualController secondary;
    const auto capture = [&]() {
        input.CaptureFrameInput(*game.GetWindow(), {primary.Get(), secondary.Get()});
        camera.ProcessInput();
        camera.Update(0.1f);
        ExpectEqual(std::size_t(2), camera.GetViews().size(), "independent camera views");
    };
    // 初期追従の補間を入力による変化と混同しないよう、両方の姿勢を先に確定する。
    camera.SnapToControlledPlayer(0, 0);
    camera.SnapToControlledPlayer(0, 1);
    capture();
    const auto initialFirst = camera.GetPlayerCameraPos(0);
    const auto initialSecond = camera.GetPlayerCameraPos(1);
    secondary.SetCameraInput(0, 12000);
    capture();
    const auto movedSecond = camera.GetPlayerCameraPos(1);
    ExpectTrue(glm::length(movedSecond - initialSecond) > 0.01f, "second controller moves second camera");
    ExpectTrue(glm::length(camera.GetPlayerCameraPos(0) - initialFirst) < 0.001f,
               "second input leaves first camera alone");
    controls.isEditorKeyboardInputCaptured = true;
    capture();
    ExpectTrue(glm::length(camera.GetPlayerCameraPos(1) - movedSecond) < 0.001f,
               "editor capture stops held second stick");
    controls.isEditorKeyboardInputCaptured = false;
    secondary.SetCameraInput(0, 0);
    capture();
    ExpectTrue(glm::length(camera.GetPlayerCameraPos(1) - movedSecond) < 0.001f, "released stick stays stopped");
    secondary.SetCameraInput(0, 12000);
    capture();
    camera.ResetForStageChange();
    camera.SnapToControlledPlayer(0, 0);
    camera.SnapToControlledPlayer(0, 1);
    camera.GetViews();
    const auto resetSecond = camera.GetPlayerCameraPos(1);
    camera.Update(0.1f);
    camera.GetViews();
    ExpectTrue(glm::length(camera.GetPlayerCameraPos(1) - resetSecond) < 0.001f,
               "stage reset clears second stick history");
    std::cout << "[PASS] Runtime.CameraControlIsolationAndEditorCapture\n";
}

} // namespace

int main(int argc, char** argv)
{
    if (argc == 4 && std::string(argv[1]) == "--frame-loop") {
        int frames = 0;
        const std::string count = argv[2];
        const auto parsed = std::from_chars(count.data(), count.data() + count.size(), frames);
        const std::string scenario = argv[3];
        if (parsed.ec != std::errc{} || parsed.ptr != count.data() + count.size() || frames < 4 || frames > 216000 ||
            (scenario != "one-player" && scenario != "two-player" && scenario != "ugc" && scenario != "cycle")) {
            std::cerr << "Expected 4..216000 frames and one-player, two-player, ugc or cycle\n";
            return 2;
        }
        try {
            Fixture fixture(true, true);
            MeasureRuntimeFrameLoop(fixture.game, frames, scenario);
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] Frame loop: " << error.what() << '\n';
            return 1;
        }
    }
    if (argc == 2 && std::string(argv[1]) == "--interactive") {
        try {
            // 実際のRunLoopを操作しつつ、既存セーブとEditor設定はテスト用保存先へ隔離する。
            Fixture fixture(true, true);
            std::cout << "Interactive validation GPU: " << glGetString(GL_RENDERER) << std::endl;
            fixture.game.RunLoop();
            const auto& metrics = fixture.game.GetFramePerformanceMetrics();
            std::cout << "Last frame snapshot (ms): total=" << metrics.totalMilliseconds
                      << " update=" << metrics.gameUpdateMilliseconds
                      << " firstViewportGPU=" << metrics.firstViewportGpuMilliseconds
                      << " hasGPU=" << metrics.hasFirstViewportGpuMeasurement << '\n';
            ExpectEqual(static_cast<GLuint>(GL_NO_ERROR), glGetError(), "interactive loop leaves no GL error");
            return 0;
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] Interactive validation: " << error.what() << '\n';
            return 1;
        }
    }

    struct Suite {
        const char* name;
        void (*run)(Game&);
    };
    const Suite suites[]{
        {"reliability", RunRuntimeReliabilityTests},
        {"gameplay", RunRuntimeGameplayTests},
        {"persistence", RunRuntimeEditorPersistenceTests},
        {"editor",
         [](Game& game) {
             DebugEditorContext context{.game = &game};
             StageSelectionController selection([&]() { return MakeTestStageSelectionScene(game); },
                                                context.gameViewport, context.planetMoveMode);
             StageEditCommandController commands(context, selection);
             EditingPersistsRuntimeUndoAndRedo(game, context, selection, commands);
         }},
        {"render",
         [](Game& game) {
             CameraManualYawRemainsIndependent(game);
             RendererDrawsStageAndBorrowedParticles(game);
             CustomUIKeepsOrderAndFitsViewport(game);
             CamerasUseExplicitContextWithoutGame();
             CameraControlsAreIndependentAndStopDuringEditorCapture(game);
             NormalCameraSeparatesPositionAndSight(game);
             SphereCameraFollowsLateralMovement(game);
             NormalCameraRestoresFramingAfterZoneExit(game, false);
             NormalCameraRestoresFramingAfterZoneExit(game, true);
             LateralWalkingKeepsNormalCameraRadius(game);
             LateralWallSlideRetainsForwardCameraMovement(game);
             LateralOrbitAssistPreservesDistanceAndTightensTurns(game);
             ScriptedPlayerMovementRestoresNormalCamera(game);
         }},
        {"ugc",
         [](Game& game) {
             DebugEditorContext context{.game = &game};
             StageSelectionController selection([&]() { return MakeTestStageSelectionScene(game); },
                                                context.gameViewport, context.planetMoveMode);
             StageEditCommandController commands(context, selection);
             UGCUsesTheSameTransaction(game, context, commands);
             UGCSceneUsesCurrentBorrowedPreviewState(game);
         }},
        {"input",
         [](Game& game) {
             InputSnapshotUsesOnlyProvidedDevices(game);
             LongPressCombatControlsUseIndependentDevices(game);
         }},
        {"combat", RunRuntimeCombatTests},
        {"lifetime",
         [](Game& game) {
             ExpectTrue(ImGui::GetCurrentContext() != nullptr, "Editor owns initialized ImGui");
             game.Shutdown();
             ExpectTrue(ImGui::GetCurrentContext() == nullptr, "Editor released before window shutdown");
             game.Shutdown();
         }},
    };
    const std::string selected = argc == 3 && std::string(argv[1]) == "--suite" ? argv[2] : "";
    if (argc != 1 && selected.empty()) {
        std::cerr << "Usage: game_runtime_tests [--suite NAME | --interactive | --frame-loop FRAMES SCENARIO]\n";
        return 2;
    }
    int failures = 0;
    int executed = 0;
    for (const auto& suite : suites) {
        if (!selected.empty() && selected != suite.name) {
            continue;
        }
        ++executed;
        try {
            {
                Fixture fixture;
                suite.run(fixture.game);
            }
            ExpectTrue(ImGui::GetCurrentContext() == nullptr, "Game destruction releases Editor context");
            if (std::string(suite.name) == "lifetime") {
                Fixture restarted(true);
                ExpectTrue(ImGui::GetCurrentContext() != nullptr, "fresh Game initializes after previous shutdown");
                TwoPlayerViewportAndEmptyTargetsRemainSafe(restarted.game);
                std::cout << "[PASS] Runtime.EditorLifetimeAndRepeatedStartup\n";
            }
            std::cout << "[PASS] Runtime suite: " << suite.name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] Runtime suite: " << suite.name << ": " << error.what() << '\n';
        }
    }
    if (executed == 0) {
        std::cerr << "Unknown Runtime test suite: " << selected << '\n';
        return 2;
    }
    std::cout << executed - failures << "/" << executed << " Runtime suites passed\n";
    return failures == 0 ? 0 : 1;
}
