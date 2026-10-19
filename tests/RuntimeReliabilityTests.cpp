#include "RuntimeReliabilityTests.h"
#include "RuntimeStageSupport.h"

#include "TestFileSupport.h"
#include "actor/Actor.h"
#include "actor/MathUtils.h"
#include "actor/Planet.h"
#include "actor/Platform.h"
#include "actor/Player.h"
#include "actor/player/PlayerParticleEffectController.h"
#include "app/Game.h"
#include "component/PlatformLatchedGroupSwitchComponent.h"
#include "component/PlatformPressureSwitchComponent.h"
#include "editor/assets/EditorAssetCatalog.h"
#include "editor/core/DebugEditorContext.h"
#include "editor/stage/StageActorYamlWriter.h"
#include "editor/stage/panels/StageAddActorPanel.h"
#include "gfx/Renderer.h"
#include "system/GamepadRumbleService.h"
#include "system/ParticleSystem.h"
#include "system/actor_loader/PlatformStageConfig.h"
#include "system/mesh/TextureLoader.h"
#include "world/Stage.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <utility>

namespace {

void SwitchStableTargetsSurviveConfigurationAndSave(Game& game)
{
    Planet planet(nullptr);
    Platform owner(&game);
    owner.SetCurrentPlanet(&planet);
    owner.SetStageSequenceName("platforms");
    owner.SetStageYamlIndex(0);
    owner.SetPlatformId("owner");
    planet.AddPlatform(&owner);
    ScopedTestStagePlanets planets(*game.GetCurrentStage(), {&planet});
    const std::vector<PlatformRevealTarget> targets{
        {"", -1, "first"}, {"", -1, "second"}, {"platforms", 0, "owner"}, {"", -1, "first"}};
    auto* pressure = owner.AddPressureSwitchComponent();
    auto* latched = owner.AddLatchedGroupSwitchComponent();
    pressure->SetHideTargets(targets);
    latched->SetRevealTargets(targets);
    latched->SetHideTargets(targets);
    ExpectEqual(std::size_t(2), pressure->GetHideTargets().size(), "distinct stable IDs are not duplicates");
    ExpectEqual(std::size_t(2), latched->GetRevealTargets().size(), "latched stable IDs exclude self and duplicates");
    ExpectEqual(std::size_t(2), latched->GetHideTargets().size(), "latched hide targets retain distinct IDs");
    DebugEditorContext context{.game = &game};
    StageActorYamlWriter writer(context);
    auto config = YAML::Load("planets: [{}]\nplatforms: [{}]\n");
    writer.WriteAllActorStates(config);
    const auto saved = ParsePlatformStageConfig(config["platforms"][0]);
    ExpectEqual(std::size_t(2), saved.behavior.pressureSwitch->hideTargets.size(), "pressure IDs survive save/reparse");
    ExpectEqual(std::string("second"), saved.behavior.pressureSwitch->hideTargets[1].platformId, "pressure stable ID");
    ExpectEqual(std::size_t(2), saved.behavior.latchedGroupSwitch->hideTargets.size(), "latched hide IDs survive save");
    ExpectEqual(std::string("second"), saved.behavior.latchedGroupSwitch->hideTargets[1].platformId, "latched hide ID");
}

void ParticlePresentationUsesOnlyProvidedSystem(Game& game)
{
    Player player(&game);
    player.SetPos({2, 3, 4});
    ParticleSystem particles(64);
    ParticleEffectDefinition effect;
    auto emitter = CreateDefaultParticleEmitter();
    emitter.count = 1;
    effect.emitters.push_back(emitter);
    for (const auto* id : {"walk_start_dust", "landing_dust", "player_special_charge"}) {
        ExpectTrue(particles.CreateEffect(id, effect), "create deterministic particle effect");
    }
    PlayerParticleEffectController presentation;
    presentation.UpdateWalking(player, particles, true);
    presentation.UpdateWalking(player, particles, true);
    ExpectEqual(1U, particles.GetParticles().size(), "one emission at walk start");
    presentation.UpdateWalking(player, particles, false);
    presentation.UpdateWalking(player, particles, true);
    ExpectEqual(2U, particles.GetParticles().size(), "new walking transition emits again");
    presentation.EmitLanding(player, particles, 0.5f);
    ExpectEqual(2U, particles.GetParticles().size(), "slow landing does not emit");
    presentation.EmitLanding(player, particles, 2);
    ExpectEqual(3U, particles.GetParticles().size(), "landing emits into provided system");
    presentation.UpdateSpecialCharging(player, particles, true, 0);
    presentation.UpdateSpecialCharging(player, particles, true, 0.05f);
    ExpectEqual(4U, particles.GetParticles().size(), "charge emission is throttled");
    presentation.UpdateSpecialCharging(player, particles, true, 0.08f);
    ExpectEqual(5U, particles.GetParticles().size(), "charge interval emits again");
    presentation.Reset();
    presentation.UpdateWalking(player, particles, true);
    presentation.UpdateSpecialCharging(player, particles, true, 0);
    ExpectEqual(7U, particles.GetParticles().size(), "reset clears both histories");
}

void ExpectBillboardBasis(const glm::mat4& matrix)
{
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            ExpectTrue(std::isfinite(matrix[column][row]), "billboard contains only finite values");
        }
    }
    const glm::vec3 x = glm::normalize(glm::vec3(matrix[0]));
    const glm::vec3 y = glm::normalize(glm::vec3(matrix[1]));
    const glm::vec3 z(matrix[2]);
    ExpectNear(1.0f, glm::length(z), 0.00001f, "unit facing axis");
    ExpectNear(0.0f, glm::dot(x, y), 0.00001f, "orthogonal horizontal and vertical axes");
    ExpectNear(0.0f, glm::dot(x, z), 0.00001f, "horizontal axis lies in billboard plane");
    ExpectNear(0.0f, glm::dot(y, z), 0.00001f, "vertical axis lies in billboard plane");
}

void BillboardDegeneracyAndNormalPose(Game& game)
{
    MathUtils math;
    const glm::mat4 view(1);
    const std::array<glm::vec3, 5> centers{
        {{0, 0, 0}, {0, -2, 0}, {0.00000001f, -2, 0}, {0, 0, -2}, {1e20f, -1e20f, 1e20f}}};
    const std::array<glm::vec3, 5> upDirections{
        {{0, 1, 0}, {0, 0, 0}, {0, 0, 1}, {0, 1e-30f, 0}, {0, std::numeric_limits<float>::infinity(), 0}}};
    for (const auto& center : centers) {
        for (const auto& up : upDirections) {
            ExpectBillboardBasis(math.CreateBillBoard(view, center, up, 2, 3));
        }
    }
    ExpectBillboardBasis(math.CreateBillBoard(glm::mat4(0), {1, 2, 3}, {0, 1, 0}, 2, 3));

    Actor actor(&game);
    actor.SetUpVec({0, 1, 0});
    actor.SetPos({0, -0.8f, 0});
    ExpectBillboardBasis(math.CreateBillBoard(view, &actor, 1, 0.25f, 2, 3));
    actor.SetPos({0, -2, 0});
    ExpectBillboardBasis(math.CreateBillBoard(view, &actor, 1, 0.25f, 2, 3));

    actor.SetPos({2, -3, -5});
    const glm::vec3 center = actor.GetPos() + actor.GetUpVec() * 0.8f;
    const glm::vec3 forward = glm::normalize(-center);
    const glm::vec3 right = glm::normalize(glm::cross(actor.GetUpVec(), forward));
    const glm::vec3 quadUp = glm::cross(forward, right);
    glm::mat4 expected(1);
    expected[0] = glm::vec4(right * 2.0f, 0);
    expected[1] = glm::vec4(-quadUp * 3.0f, 0);
    expected[2] = glm::vec4(forward, 0);
    expected[3] = glm::vec4(actor.GetPos() + actor.GetUpVec() + right * 0.25f, 1);
    const auto actual = math.CreateBillBoard(view, &actor, 1, 0.25f, 2, 3);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            ExpectNear(expected[column][row], actual[column][row], 0.00001f, "normal Actor billboard unchanged");
        }
    }
    const auto positioned = math.CreateBillBoard(view, center, {0, 1, 0}, 2, 3);
    for (int column = 0; column < 3; ++column) {
        for (int row = 0; row < 4; ++row) {
            ExpectNear(expected[column][row], positioned[column][row], 0.00001f, "normal positioned basis unchanged");
        }
    }
}

class VirtualDevice {
public:
    VirtualDevice()
    {
        const int index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX,
                                                    SDL_CONTROLLER_BUTTON_MAX, 0);
        ExpectTrue(index >= 0 && SDL_IsGameController(index), "attach virtual game controller");
        mId = SDL_JoystickGetDeviceInstanceID(index);
    }
    ~VirtualDevice() { Detach(); }
    void Detach()
    {
        for (int index = 0; index < SDL_NumJoysticks(); ++index) {
            if (SDL_JoystickGetDeviceInstanceID(index) == mId) {
                SDL_JoystickDetachVirtual(index);
                break;
            }
        }
        SDL_JoystickUpdate();
    }
    SDL_JoystickID Id() const { return mId; }

private:
    SDL_JoystickID mId = -1;
};

SDL_JoystickID ControllerId(const GamepadRumbleService& service, int playerNumber)
{
    SDL_GameController* controller = service.GetControllerForPlayer(playerNumber);
    ExpectTrue(controller && SDL_GameControllerGetAttached(controller), "Player has an attached controller");
    return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));
}

void ControllerReplacementKeepsCurrentDeviceOrder(Game&)
{
    int realControllers = 0;
    for (int index = 0; index < SDL_NumJoysticks(); ++index) {
        realControllers += SDL_IsGameController(index) ? 1 : 0;
    }
    VirtualDevice first;
    VirtualDevice second;
    GamepadRumbleService service;
    service.UpdateConnection();
    ExpectEqual(first.Id(), ControllerId(service, realControllers + 1), "first virtual Player slot");
    ExpectEqual(second.Id(), ControllerId(service, realControllers + 2), "second virtual Player slot");
    auto* stable = service.GetControllerForPlayer(realControllers + 2);
    service.UpdateConnection();
    ExpectTrue(stable == service.GetControllerForPlayer(realControllers + 2), "unchanged devices keep handles");

    first.Detach();
    VirtualDevice replacement;
    service.UpdateConnection();
    ExpectEqual(second.Id(), ControllerId(service, realControllers + 1), "survivor follows SDL device order");
    ExpectEqual(replacement.Id(), ControllerId(service, realControllers + 2), "same-count replacement detected");
    second.Detach();
    service.UpdateConnection();
    ExpectEqual(replacement.Id(), ControllerId(service, realControllers + 1), "two-to-one compacts Player slots");
    ExpectTrue(service.GetControllerForPlayer(realControllers + 2) == nullptr, "removed slot cleared");
    VirtualDevice reconnected;
    service.UpdateConnection();
    ExpectEqual(reconnected.Id(), ControllerId(service, realControllers + 2), "reconnection receives new instance");
}

struct Texture {
    GLuint id = 0;
    ~Texture() { glDeleteTextures(1, &id); }
};

struct RestorePixelState {
    GLint binding = 0;
    GLint unpack = 0;
    GLint pack = 0;
    GLint buffer = 0;
    GLint rowLength = 0;
    GLint skipRows = 0;
    GLint skipPixels = 0;
    RestorePixelState()
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
        glGetIntegerv(GL_PACK_ALIGNMENT, &pack);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &buffer);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &rowLength);
        glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skipRows);
        glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skipPixels);
    }
    ~RestorePixelState()
    {
        glBindTexture(GL_TEXTURE_2D, binding);
        glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
        glPixelStorei(GL_PACK_ALIGNMENT, pack);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rowLength);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, skipRows);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, skipPixels);
    }
};

std::filesystem::path WriteStripedImage(const TemporaryTestDirectory& directory)
{
    const unsigned char pixels[]{255, 0, 0, 255, 0, 0, 255, 0, 0, 0, 0, 255, 0, 0, 255, 0, 0, 255};
    const auto path = directory.Root() / "striped.ppm";
    std::ofstream out(path, std::ios::binary);
    out << "P6\n3 2\n255\n";
    out.write(reinterpret_cast<const char*>(pixels), sizeof(pixels));
    out.close();
    ExpectTrue(static_cast<bool>(out), "write exact RGB test image");
    return path;
}

void TextureLoadingPreservesCallerState(Game&)
{
    RestorePixelState restore;
    TemporaryTestDirectory directory;
    const auto path = WriteStripedImage(directory);
    Texture sentinel;
    glGenTextures(1, &sentinel.id);
    glBindTexture(GL_TEXTURE_2D, sentinel.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    struct UnpackBuffer {
        GLuint id = 0;
        ~UnpackBuffer() { glDeleteBuffers(1, &id); }
    } buffer;
    glGenBuffers(1, &buffer.id);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer.id);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 1024, nullptr, GL_STATIC_DRAW);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 9);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 2);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1);
    TextureLoader loader;
    Texture image{loader.LoadTexture(path.string().c_str(), TextureColorSpace::Linear)};
    ExpectTrue(image.id != 0, "load test image");
    GLint alignment = 0;
    GLint binding = 0;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    ExpectEqual(8, alignment, "restore unpack alignment");
    ExpectEqual(sentinel.id, static_cast<GLuint>(binding), "restore texture binding");
    for (const auto& [state, expected] :
         std::array<std::pair<GLenum, GLint>, 4>{{{GL_PIXEL_UNPACK_BUFFER_BINDING, static_cast<GLint>(buffer.id)},
                                                  {GL_UNPACK_ROW_LENGTH, 9},
                                                  {GL_UNPACK_SKIP_ROWS, 2},
                                                  {GL_UNPACK_SKIP_PIXELS, 1}}}) {
        GLint actual = 0;
        glGetIntegerv(state, &actual);
        ExpectEqual(expected, actual, "restore PBO and row layout");
    }
    ExpectEqual(static_cast<GLenum>(GL_NO_ERROR), glGetError(), "CPU upload ignores caller PBO and row layout");
    ExpectEqual(0U, loader.LoadTexture("missing-test-image.png", TextureColorSpace::Linear), "missing image fails");
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
    ExpectEqual(sentinel.id, static_cast<GLuint>(binding), "failed load also preserves binding");
}

class TextureProbeRenderer : public Renderer {
public:
    GLuint Load(const std::string& path)
    {
        RegisterTexture(path, "orientation-regression");
        return mTextures.at("orientation-regression");
    }
};

void TextureOrientationDoesNotLeakIntoUI(Game&)
{
    RestorePixelState restore;
    TemporaryTestDirectory directory;
    const auto path = WriteStripedImage(directory);
    TextureLoader loader;
    Texture image{loader.LoadTexture(path.string().c_str(), TextureColorSpace::Linear)};
    TextureProbeRenderer renderer;
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    std::array<unsigned char, 18> pixels{};
    glBindTexture(GL_TEXTURE_2D, image.id);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    ExpectEqual(255, static_cast<int>(pixels[2]), "model texture keeps bottom-to-top row orientation");
    glBindTexture(GL_TEXTURE_2D, renderer.Load(path.string()));
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    ExpectEqual(255, static_cast<int>(pixels[0]), "UI texture keeps startup top-to-bottom orientation");
    ExpectEqual(static_cast<GLenum>(GL_NO_ERROR), glGetError(), "image upload and readback have no GL error");
}

void EnemyFormFailureDoesNotHideOtherForms(Game& game)
{
    Stage& stage = *game.GetCurrentStage();
    struct RestorePlanets {
        Stage& stage;
        std::vector<Planet*> planets;
        ~RestorePlanets()
        {
            for (auto* planet : planets) {
                stage.AddPlanet(planet);
            }
        }
    } restore{stage, stage.GetPlanets()};
    stage.RemoveAllPlanet();
    EditorAssetCatalog assets;
    DebugEditorContext context{.game = &game};
    context.assetCatalog = &assets;
    StageAddActorPanel panel(context);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::GetIO().DisplaySize = ImVec2(1280, 720);
    ImGui::NewFrame();
    ImGui::SetNextWindowSize(ImVec2(1000, 700));
    ImGui::Begin("Independent creation forms");
    ImGui::GetStateStorage()->SetInt(ImGui::GetID("敵追加"), 1);
    const ImGuiID finalForm = ImGui::GetID("星追加");
    panel.Draw();
    const ImGuiID lastItem = ImGui::GetItemID();
    ImGui::End();
    ImGui::EndFrame();
    ExpectEqual(finalForm, lastItem, "forms after failed enemy form are still submitted");
}

} // namespace

void RunRuntimeReliabilityTests(Game& game)
{
    int failures = 0;
    const std::pair<const char*, void (*)(Game&)> cases[]{
        {"SwitchStableTargetsSurviveConfigurationAndSave", SwitchStableTargetsSurviveConfigurationAndSave},
        {"ParticlePresentationUsesOnlyProvidedSystem", ParticlePresentationUsesOnlyProvidedSystem},
        {"BillboardDegeneracyAndNormalPose", BillboardDegeneracyAndNormalPose},
        {"ControllerReplacementKeepsCurrentDeviceOrder", ControllerReplacementKeepsCurrentDeviceOrder},
        {"TextureLoadingPreservesCallerState", TextureLoadingPreservesCallerState},
        {"TextureOrientationDoesNotLeakIntoUI", TextureOrientationDoesNotLeakIntoUI},
        {"EnemyFormFailureDoesNotHideOtherForms", EnemyFormFailureDoesNotHideOtherForms},
    };
    for (const auto& [name, run] : cases) {
        try {
            run(game);
            std::cout << "[PASS] Runtime." << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] Runtime." << name << ": " << error.what() << '\n';
        }
    }
    ExpectEqual(0, failures, "runtime reliability regressions");
}
