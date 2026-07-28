#include "CameraSystem.h"

#include "Game.h"
#include "actor/Enemy.h"
#include "actor/Key.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "actor/Star.h"
#include "component/FocusComponent.h"
#include "system/ActorLoadSystem.h"
#include "system/SceneSystem.h"

#include <GLFW/glfw3.h>
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

CameraSystem::CameraSystem(Game* game)
    : mGame(game),
      mCollisionResolver(game),
      mDebugCamera(game),
      mFocusCamera(game),
      mPlayerCamera(mCollisionResolver),
      mPlayerCameraSettingsRepository("../assets/data/camera/player.yaml"),
      mCinematicLibrary("../assets/data/camera/cinematics.yaml")
{
    mPlayerCameraSettingsRepository.Load(mPlayerCameraSettings);
    mCinematicLibrary.Load();
}

void CameraSystem::ProcessInput()
{
    if (!mGame || mCinematicCamera.IsPlaying()) {
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.ProcessInput();
        return;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    SDL_GameController* sdlController = mGame->GetSdlController();

    constexpr float deadZone = 0.25f;
    constexpr float scale = 1.0f / 32767.0f;

    mCameraStickX =
        sdlController
            ? SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTX) * scale
            : 0.0f;
    mCameraStickY =
        sdlController
            ? SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_RIGHTY) * scale
            : 0.0f;

    if (std::abs(mCameraStickX) < deadZone) {
        mCameraStickX = 0.0f;
    }
    if (std::abs(mCameraStickY) < deadZone) {
        mCameraStickY = 0.0f;
    }

    mKeyboardPitchInput = 0.0f;
    GLFWwindow* window = mGame->GetWindow();
    if (window && glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        mKeyboardPitchInput += 1.0f;
    }
    if (window && glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        mKeyboardPitchInput -= 1.0f;
    }

    constexpr Sint16 triggerPressedThreshold = 16000;
    const bool alignCameraPressed =
        (mGame->GetWindow() && glfwGetKey(mGame->GetWindow(), GLFW_KEY_Y) == GLFW_PRESS) ||
        (sdlController &&
         SDL_GameControllerGetAxis(sdlController, SDL_CONTROLLER_AXIS_TRIGGERLEFT) >
             triggerPressedThreshold);

    if (sceneSystem && sceneSystem->IsPlaying() && alignCameraPressed && !mAlignCameraPressedPrev) {
        mPlayerCamera.AlignBehindPlayer(mGame->GetMainPlayer(), 0);
    }

    mAlignCameraPressedPrev = alignCameraPressed;
}

void CameraSystem::Update(float deltaTime)
{
    UpdateCamera(deltaTime);
}

bool CameraSystem::PlayCinematic(std::string_view sequenceId)
{
    const CinematicSequence* sequence = mCinematicLibrary.Find(sequenceId);
    return sequence && mCinematicCamera.Play(*sequence);
}

void CameraSystem::StopCinematic()
{
    mCinematicCamera.Stop();
}

bool CameraSystem::ReloadCinematicSequences()
{
    StopCinematic();
    return mCinematicLibrary.Load();
}

void CameraSystem::SetPlayerCameraSettings(PlayerCameraSettings settings)
{
    settings.Normalize();
    mPlayerCameraSettings = settings;
}

bool CameraSystem::SavePlayerCameraSettings() const
{
    return mPlayerCameraSettingsRepository.Save(mPlayerCameraSettings);
}

bool CameraSystem::ReloadPlayerCameraSettings()
{
    PlayerCameraSettings settings;
    if (!mPlayerCameraSettingsRepository.Load(settings)) {
        return false;
    }

    mPlayerCameraSettings = settings;
    return true;
}

void CameraSystem::ResetForStageChange()
{
    StopCinematic();
    StopBossDefeatSequence();

    mIsTargetFocus = false;
    mAlignCameraPressedPrev = false;
    mCameraStickX = 0.0f;
    mCameraStickY = 0.0f;
    mKeyboardPitchInput = 0.0f;
    mPlayerPitchOffsetsDegrees.clear();
    mTalkCameraBlend = 0.0f;
    mTalkCameraPlayer = nullptr;
    mHasTalkCameraTarget = false;
    mTalkCameraTargetPos = glm::vec3(0.0f);
    mTalkPageFocusActor = nullptr;
    mTalkPageFocusBlend = 0.0f;
    mHasTalkPageFocusPose = false;
    mTalkPageFocusCameraPos = glm::vec3(0.0f);
    mTalkPageFocusTargetPos = glm::vec3(0.0f);
    mTalkPageFocusUpVec = glm::vec3(0.0f, 1.0f, 0.0f);
    mRenderedTalkPageCameraPos = glm::vec3(0.0f);
}

float CameraSystem::GetFieldOfViewDegrees() const
{
    if (mCinematicCamera.IsPlaying()) {
        return glm::clamp(mCinematicCamera.GetPose().fieldOfViewDegrees, 10.0f, 120.0f);
    }

    if (mGame && mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetFieldOfViewDegrees();
    }

    if (mBossDefeatSequenceTimer >= 0.0f) {
        return mPlayerCameraSettings.bossDefeatFieldOfViewDegrees;
    }

    const float normalFieldOfView =
        mGame && mGame->GetIsPlayer2Joined()
            ? mPlayerCameraSettings.splitScreenFieldOfViewDegrees
            : mPlayerCameraSettings.fieldOfViewDegrees;

    return glm::mix(
        normalFieldOfView, mPlayerCameraSettings.talkFieldOfViewDegrees, GetEasedTalkCameraBlend());
}

void CameraSystem::UpdateCamera(float deltaTime)
{
    if (!mGame) {
        return;
    }

    UpdateTalkCameraTransition(deltaTime);
    UpdateBossDefeatSequence(deltaTime);

    if (mCinematicCamera.IsPlaying()) {
        mCinematicCamera.Update(deltaTime);
        return;
    }

    if (mGame->GetIsFreeCameraMode()) {
        mDebugCamera.Update(deltaTime);
        return;
    }

    const float yawDelta = mTalkCameraBlend > 0.0f
                               ? 0.0f
                               : mCameraStickX * mPlayerCameraSettings.yawSensitivity * deltaTime;

    UpdatePlayerPitchOffsets(deltaTime);

    mPlayerCamera.Update(mGame->GetPlayers(), yawDelta, mPlayerCameraSettings.upSmoothingSpeed,
                         mPlayerCameraSettings.targetSmoothingSpeed,
                         mPlayerCameraSettings.attackTargetSmoothingSpeed, deltaTime);
    UpdateTalkCameraAim();
    UpdateTalkPageFocus(deltaTime);
}

void CameraSystem::UpdatePlayerPitchOffsets(float deltaTime)
{
    if (!mGame) {
        return;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (mPlayerPitchOffsetsDegrees.size() < players.size()) {
        mPlayerPitchOffsetsDegrees.resize(players.size(), 0.0f);
    }

    const bool allowsManualPitch =
        mTalkCameraBlend <= 0.0f &&
        mBossDefeatSequenceTimer < 0.0f &&
        !mIsTargetFocus;

    if (allowsManualPitch) {
        const float pitchSpeed =
            mPlayerCameraSettings.pitchSensitivityDegrees * std::max(0.0f, deltaTime);

        if (!players.empty() && players[0] && mGame->IsGameControllerConnected()) {
            mPlayerPitchOffsetsDegrees[0] += -mCameraStickY * pitchSpeed;
        }

        int keyboardPlayerIndex = -1;
        if (mGame->IsGameControllerConnected()) {
            if (players.size() >= 2 && players[1]) {
                keyboardPlayerIndex = 1;
            }
        } else if (!players.empty() && players[0]) {
            keyboardPlayerIndex = 0;
        }

        if (keyboardPlayerIndex >= 0) {
            mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(keyboardPlayerIndex)] +=
                mKeyboardPitchInput * pitchSpeed;
        }
    }

    for (float& pitchOffset : mPlayerPitchOffsetsDegrees) {
        const float pitch = glm::clamp(
            mPlayerCameraSettings.pitchDegrees + pitchOffset,
            mPlayerCameraSettings.minPitchDegrees,
            mPlayerCameraSettings.maxPitchDegrees);
        pitchOffset = pitch - mPlayerCameraSettings.pitchDegrees;
    }
}

bool CameraSystem::AllowsPlayerInput() const
{
    return mBossDefeatSequenceTimer < 0.0f && !mCinematicCamera.IsPlaying() &&
           !(mGame && mGame->GetIsFreeCameraMode()) && !mIsTargetFocus;
}

void CameraSystem::StartBossDefeatSequence(Enemy* boss, Star* star)
{
    if (!boss) {
        return;
    }

    mDefeatedBoss = boss;
    mBossDefeatStar = star;
    mBossDefeatSequenceIsPreview = false;
    mBossDefeatSequenceTimer = 0.0f;
    mCameraStickX = 0.0f;

    Player* player = mGame ? mGame->GetMainPlayer() : nullptr;
    const glm::vec3 targetPos = player ? player->GetPos() : boss->GetPos();
    const glm::vec3 up = player ? player->GetUpVec() : boss->GetUpVec();
    mFocusCamera.BeginTransition(mPlayerCamera.GetCameraPos(0), targetPos, up);
}

bool CameraSystem::PreviewBossDefeatSequence()
{
    Player* player = mGame ? mGame->GetMainPlayer() : nullptr;
    Planet* planet = player ? player->GetCurrentPlanet() : nullptr;
    Enemy* boss = FindBossEnemy(planet);
    if (!boss) {
        return false;
    }

    StartBossDefeatSequence(boss, planet->GetStar());
    mBossDefeatSequenceIsPreview = true;
    return true;
}

void CameraSystem::StopBossDefeatSequence()
{
    mBossDefeatSequenceTimer = -1.0f;
    mBossDefeatSequenceIsPreview = false;
    mDefeatedBoss = nullptr;
    mBossDefeatStar = nullptr;
}

void CameraSystem::UpdateBossDefeatSequence(float deltaTime)
{
    if (mBossDefeatSequenceTimer < 0.0f) {
        return;
    }

    mBossDefeatSequenceTimer += std::max(0.0f, deltaTime);

    constexpr float starRevealTime = 4.0f;
    if (!mBossDefeatSequenceIsPreview && mBossDefeatStar &&
        mBossDefeatSequenceTimer >= starRevealTime && !mBossDefeatStar->GetIsActive()) {
        mBossDefeatStar->SetIsActive(true);
    }

    constexpr float sequenceDuration = 7.0f;
    if (mBossDefeatSequenceTimer < sequenceDuration) {
        return;
    }

    StopBossDefeatSequence();
}

void CameraSystem::UpdateTalkCameraTransition(float deltaTime)
{
    SceneSystem* sceneSystem = mGame ? mGame->GetSceneSystem() : nullptr;

    Player* targetPlayer = nullptr;
    NPC* targetNPC = nullptr;
    if (sceneSystem && sceneSystem->IsTalkWithNPC() && sceneSystem->GetTalkingPlayer()) {
        targetPlayer = sceneSystem->GetTalkingPlayer();
        targetNPC = sceneSystem->GetTalkingNPC();
    } else if (mTalkCameraPreviewEnabled && mGame) {
        targetPlayer = mGame->GetMainPlayer();
    }

    if (targetPlayer) {
        if (mTalkCameraPlayer && mTalkCameraPlayer != targetPlayer) {
            mTalkCameraBlend = 0.0f;
        }

        mTalkCameraPlayer = targetPlayer;
        if (targetNPC) {
            mTalkCameraTargetPos = targetNPC->GetPos();
            mHasTalkCameraTarget = true;
        }

        const float duration = mPlayerCameraSettings.talkTransitionInDuration;
        mTalkCameraBlend =
            duration <= 0.0f ? 1.0f : std::min(1.0f, mTalkCameraBlend + std::max(0.0f, deltaTime) / duration);
        return;
    }

    const float duration = mPlayerCameraSettings.talkTransitionOutDuration;
    mTalkCameraBlend =
        duration <= 0.0f ? 0.0f : std::max(0.0f, mTalkCameraBlend - std::max(0.0f, deltaTime) / duration);

    if (mTalkCameraBlend <= 0.0f) {
        mTalkCameraPlayer = nullptr;
        mHasTalkCameraTarget = false;
    }
}

void CameraSystem::UpdateTalkCameraAim()
{
    if (!mGame || !mTalkCameraPlayer || !mHasTalkCameraTarget || mTalkCameraBlend <= 0.0f) {
        return;
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    for (int playerIndex = 0; playerIndex < static_cast<int>(players.size()); ++playerIndex) {
        if (players[playerIndex] != mTalkCameraPlayer) {
            continue;
        }

        mPlayerCamera.BlendBehindTarget(
            mTalkCameraPlayer, playerIndex, mTalkCameraTargetPos, GetEasedTalkCameraBlend());
        return;
    }
}

void CameraSystem::UpdateTalkPageFocus(float deltaTime)
{
    Actor* desiredFocusActor = ResolveTalkPageFocusActor();
    if (desiredFocusActor && !desiredFocusActor->GetIsActive()) {
        desiredFocusActor = nullptr;
    }

    const float safeDeltaTime = std::max(0.0f, deltaTime);
    if (desiredFocusActor) {
        if (!mHasTalkPageFocusPose) {
            Player* player = mTalkCameraPlayer;
            glm::vec3 up = player ? player->GetUpVec() : desiredFocusActor->GetUpVec();
            if (glm::dot(up, up) < 0.000001f) {
                up = glm::vec3(0.0f, 1.0f, 0.0f);
            } else {
                up = glm::normalize(up);
            }

            mTalkPageFocusCameraPos = mPlayerCamera.GetCameraPos(0);
            if (glm::dot(mTalkPageFocusCameraPos, mTalkPageFocusCameraPos) < 0.000001f) {
                mTalkPageFocusCameraPos =
                    (player ? player->GetPos() : desiredFocusActor->GetPos()) + up * 3.0f;
            }
            mTalkPageFocusTargetPos =
                (player ? player->GetPos() : desiredFocusActor->GetPos()) +
                up * mPlayerCameraSettings.talkTargetHeight;
            mTalkPageFocusUpVec = up;
            mHasTalkPageFocusPose = true;
        }

        mTalkPageFocusActor = desiredFocusActor;

        const float duration = mPlayerCameraSettings.talkTransitionInDuration;
        mTalkPageFocusBlend =
            duration <= 0.0f
                ? 1.0f
                : std::min(1.0f, mTalkPageFocusBlend + safeDeltaTime / duration);

        glm::vec3 up = desiredFocusActor->GetUpVec();
        if (glm::dot(up, up) < 0.000001f) {
            up = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            up = glm::normalize(up);
        }

        glm::vec3 back =
            (mTalkCameraPlayer ? mTalkCameraPlayer->GetPos() : desiredFocusActor->GetPos()) -
            desiredFocusActor->GetPos();
        back -= up * glm::dot(back, up);
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));
        }
        if (glm::dot(back, back) < 0.000001f) {
            back = glm::cross(up, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        back = glm::normalize(back);

        const glm::vec3 desiredTargetPos =
            desiredFocusActor->GetPos() + up * mPlayerCameraSettings.talkTargetHeight;
        const glm::vec3 uncorrectedCameraPos =
            desiredTargetPos +
            back * mPlayerCameraSettings.talkDistance +
            up * (mPlayerCameraSettings.talkDistance * 0.35f);
        const glm::vec3 desiredCameraPos =
            mCollisionResolver.Resolve(desiredTargetPos, uncorrectedCameraPos);

        constexpr float focusPoseSmoothingSpeed = 7.0f;
        const float poseBlend =
            1.0f - std::exp(-focusPoseSmoothingSpeed * safeDeltaTime);
        mTalkPageFocusCameraPos =
            glm::mix(mTalkPageFocusCameraPos, desiredCameraPos, poseBlend);
        mTalkPageFocusTargetPos =
            glm::mix(mTalkPageFocusTargetPos, desiredTargetPos, poseBlend);
        const glm::vec3 blendedUp = glm::mix(mTalkPageFocusUpVec, up, poseBlend);
        if (glm::dot(blendedUp, blendedUp) >= 0.000001f) {
            mTalkPageFocusUpVec = glm::normalize(blendedUp);
        }
        return;
    }

    mTalkPageFocusActor = nullptr;
    const float duration = mPlayerCameraSettings.talkTransitionOutDuration;
    mTalkPageFocusBlend =
        duration <= 0.0f
            ? 0.0f
            : std::max(0.0f, mTalkPageFocusBlend - safeDeltaTime / duration);

    if (mTalkPageFocusBlend <= 0.0f) {
        mHasTalkPageFocusPose = false;
    }
}

float CameraSystem::GetEasedTalkCameraBlend() const
{
    const float blend = glm::clamp(mTalkCameraBlend, 0.0f, 1.0f);
    return blend * blend * (3.0f - 2.0f * blend);
}

float CameraSystem::GetEasedTalkPageFocusBlend() const
{
    const float blend = glm::clamp(mTalkPageFocusBlend, 0.0f, 1.0f);
    return blend * blend * (3.0f - 2.0f * blend);
}

glm::mat4 CameraSystem::GetPlayerCameraView(Player* player, int playerIndex)
{
    const float talkBlend = player && player == mTalkCameraPlayer ? GetEasedTalkCameraBlend() : 0.0f;
    const float distance =
        glm::mix(mPlayerCameraSettings.distance, mPlayerCameraSettings.talkDistance, talkBlend);
    const float pitchOffset =
        playerIndex >= 0 &&
        static_cast<std::size_t>(playerIndex) < mPlayerPitchOffsetsDegrees.size()
            ? mPlayerPitchOffsetsDegrees[static_cast<std::size_t>(playerIndex)]
            : 0.0f;
    const float normalPitchDegrees = glm::clamp(
        mPlayerCameraSettings.pitchDegrees + pitchOffset,
        mPlayerCameraSettings.minPitchDegrees,
        mPlayerCameraSettings.maxPitchDegrees);
    const float pitchDegrees =
        glm::mix(normalPitchDegrees, mPlayerCameraSettings.talkPitchDegrees, talkBlend);
    const float targetHeight =
        glm::mix(mPlayerCameraSettings.targetHeight, mPlayerCameraSettings.talkTargetHeight, talkBlend);

    return mPlayerCamera.GetView(
        player, playerIndex, distance, glm::radians(pitchDegrees), targetHeight);
}

glm::mat4 CameraSystem::GetTalkPageFocusView(Player* player, int playerIndex)
{
    const glm::mat4 normalView = GetPlayerCameraView(player, playerIndex);
    if (!player || !mHasTalkPageFocusPose || mTalkPageFocusBlend <= 0.0f) {
        return normalView;
    }

    glm::vec3 normalUp = player->GetUpVec();
    if (glm::dot(normalUp, normalUp) < 0.000001f) {
        normalUp = glm::vec3(0.0f, 1.0f, 0.0f);
    } else {
        normalUp = glm::normalize(normalUp);
    }

    const glm::vec3 normalCameraPos = mPlayerCamera.GetCameraPos(playerIndex);
    const glm::vec3 normalTargetPos =
        player->GetPos() + normalUp * mPlayerCameraSettings.talkTargetHeight;
    const float blend = GetEasedTalkPageFocusBlend();

    const glm::vec3 cameraPos =
        glm::mix(normalCameraPos, mTalkPageFocusCameraPos, blend);
    const glm::vec3 targetPos =
        glm::mix(normalTargetPos, mTalkPageFocusTargetPos, blend);
    glm::vec3 up = glm::mix(normalUp, mTalkPageFocusUpVec, blend);
    if (glm::dot(up, up) < 0.000001f) {
        up = normalUp;
    } else {
        up = glm::normalize(up);
    }

    mRenderedTalkPageCameraPos = cameraPos;
    return glm::lookAt(cameraPos, targetPos, up);
}

Actor* CameraSystem::ResolveTalkPageFocusActor() const
{
    if (!mGame || !mTalkCameraPlayer) {
        return nullptr;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    NPC* talkingNPC = sceneSystem ? sceneSystem->GetTalkingNPC() : nullptr;
    if (!sceneSystem || !sceneSystem->IsTalkWithNPC() || !talkingNPC) {
        return nullptr;
    }

    const int talkIndex = sceneSystem->GetTalkUIIndex();
    if (talkIndex < 0) {
        return nullptr;
    }

    const NPCTalkCameraFocusTarget* target =
        talkingNPC->GetTalkCameraFocusTarget(static_cast<std::size_t>(talkIndex));
    if (!target || !target->IsValid()) {
        return nullptr;
    }

    ActorLoadSystem* actorLoadSystem = mGame->GetActorLoadSystem();
    return actorLoadSystem
               ? actorLoadSystem->FindPlacedActor(target->sequenceName, target->yamlIndex)
               : nullptr;
}

std::vector<glm::mat4> CameraSystem::GetViews()
{
    std::vector<glm::mat4> views;

    if (!mGame) {
        return views;
    }

    if (mCinematicCamera.IsPlaying()) {
        views.emplace_back(mCinematicCamera.GetView());
        return views;
    }

    if (mGame->GetIsFreeCameraMode()) {
        views.emplace_back(mDebugCamera.GetView());
        return views;
    }

    if (mBossDefeatSequenceTimer >= 0.0f) {
        constexpr float starRevealTime = 4.0f;
        if (mBossDefeatSequenceTimer < starRevealTime && mDefeatedBoss) {
            views.emplace_back(mFocusCamera.GetCloseFocusView(
                mDefeatedBoss, mPlayerCameraSettings.bossDefeatDistance,
                mPlayerCameraSettings.bossDefeatCameraHeight,
                mPlayerCameraSettings.bossDefeatTargetHeight));
        } else if (mBossDefeatStar) {
            views.emplace_back(mFocusCamera.GetCloseFocusView(
                mBossDefeatStar, mPlayerCameraSettings.bossDefeatStarDistance,
                mPlayerCameraSettings.bossDefeatStarCameraHeight,
                mPlayerCameraSettings.bossDefeatStarTargetHeight));
        }

        if (!views.empty()) {
            return views;
        }
    }

    const std::vector<Player*>& players = mGame->GetPlayers();
    if (players.empty() || !players[0]) {
        return views;
    }

    SceneSystem* sceneSystem = mGame->GetSceneSystem();
    if (sceneSystem && sceneSystem->IsOpening()) {
        views = mFocusCamera.GetOpeningViews();
        if (!views.empty()) {
            return views;
        }
    }

    Planet* currentPlanet = players[0]->GetCurrentPlanet();
    if (!currentPlanet) {
        return views;
    }

    const bool isPlayer2Joined = mGame->GetIsPlayer2Joined() && players.size() >= 2 && players[1];
    if ((mTalkCameraPlayer && mTalkCameraBlend > 0.0f) ||
        (mHasTalkPageFocusPose && mTalkPageFocusBlend > 0.0f)) {
        views.emplace_back(GetTalkPageFocusView(players[0], 0));
        if (isPlayer2Joined) {
            views.emplace_back(GetPlayerCameraView(players[1], 1));
        }
        return views;
    }

    const std::vector<Boat*> boats = currentPlanet->GetBoats();
    if (!boats.empty()) {
        views = mFocusCamera.GetBoatFocusViews(boats);
        if (!views.empty()) {
            return views;
        }
    }

    Key* key = currentPlanet->GetKey();
    if (key) {
        FocusComponent* focusComponent = key->GetFocusComponent();
        if (focusComponent && focusComponent->GetFocusTimer() >= 0.0f) {
            views.emplace_back(mFocusCamera.GetFocusView(key));
            return views;
        }
    }

    if (sceneSystem && sceneSystem->IsStageClear()) {
        views.emplace_back(mPlayerCamera.GetView(players[0], 0, 6.0f, -1.0f, 1.0f, true));
        return views;
    }

    if (mIsTargetFocus) {
        Enemy* targetEnemy = FindBossEnemy(currentPlanet);
        if (targetEnemy) {
            views.emplace_back(mFocusCamera.GetTargetCameraView(targetEnemy));
            return views;
        }
    }

    views.emplace_back(GetPlayerCameraView(players[0], 0));

    if (isPlayer2Joined) {
        views.emplace_back(GetPlayerCameraView(players[1], 1));
    }

    return views;
}

glm::vec3 CameraSystem::GetCameraPos() const
{
    if (!mGame) {
        return glm::vec3(0.0f);
    }

    if (mCinematicCamera.IsPlaying()) {
        return mCinematicCamera.GetPose().position;
    }

    if (mGame->GetIsFreeCameraMode()) {
        return mDebugCamera.GetCameraPos();
    }

    if (mHasTalkPageFocusPose && mTalkPageFocusBlend > 0.0f) {
        return mRenderedTalkPageCameraPos;
    }

    if (mTalkCameraPlayer && mTalkCameraBlend > 0.0f) {
        return mPlayerCamera.GetCameraPos(0);
    }

    if (mIsTargetFocus) {
        return mFocusCamera.GetCameraPos();
    }

    return mPlayerCamera.GetCameraPos(0);
}

glm::vec3 CameraSystem::GetPlayerCameraPos(int playerNum) const
{
    return mPlayerCamera.GetCameraPos(playerNum);
}

Enemy* CameraSystem::FindBossEnemy(Planet* planet) const
{
    if (!planet) {
        return nullptr;
    }

    const std::vector<Enemy*> enemies = planet->GetEnemies();
    for (Enemy* enemy : enemies) {
        if (!enemy || !enemy->GetIsBoss()) {
            continue;
        }

        return enemy;
    }

    return nullptr;
}
