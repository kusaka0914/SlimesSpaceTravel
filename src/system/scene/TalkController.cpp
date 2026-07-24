#include "system/scene/TalkController.h"

#include "Game.h"
#include "actor/NPC.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include <vector>

namespace {
constexpr float directionEpsilonSquared = 0.000001f;

bool TryGetTangentDirection(const glm::vec3& direction, const glm::vec3& up, glm::vec3& tangentDirection)
{
    const glm::vec3 projectedDirection = direction - up * glm::dot(direction, up);
    const float lengthSquared = glm::dot(projectedDirection, projectedDirection);
    if (lengthSquared <= directionEpsilonSquared) {
        return false;
    }

    tangentDirection = projectedDirection / std::sqrt(lengthSquared);
    return true;
}

glm::vec3 RotateTowards(const glm::vec3& current, const glm::vec3& desired, const glm::vec3& up,
                        float smoothingSpeed, float deltaTime)
{
    const float dotValue = glm::clamp(glm::dot(current, desired), -1.0f, 1.0f);
    const float signedAngle = std::atan2(glm::dot(glm::cross(current, desired), up), dotValue);
    const float blend =
        1.0f - std::exp(-std::max(0.0f, smoothingSpeed) * std::max(0.0f, deltaTime));
    const float angle = signedAngle * blend;
    const glm::vec3 rotated =
        current * std::cos(angle) + glm::cross(up, current) * std::sin(angle);

    glm::vec3 tangentDirection;
    return TryGetTangentDirection(rotated, up, tangentDirection) ? tangentDirection : desired;
}
} // namespace

TalkController::TalkController(Game* game, GameProgressState* gameProgressState, UIState* uiState, NPC*& talkingNPC,
                               Player*& talkingPlayer)
    : mGame(game),
      mGameProgressState(gameProgressState),
      mUIState(uiState),
      mTalkingNPC(talkingNPC),
      mTalkingPlayer(talkingPlayer)
{
}

void TalkController::Update(float deltaTime)
{
    if (mGameProgressState->GetSceneState() != GameProgressState::SceneState::Talking ||
        mUIState->GetCurrentTalkWith() != UIState::TalkWith::NPC ||
        !mTalkingNPC || !mTalkingPlayer ||
        !mTalkingNPC->GetIsActive() || !mTalkingPlayer->GetIsActive()) {
        return;
    }

    glm::vec3 up = mTalkingPlayer->GetUpVec();
    const float upLengthSquared = glm::dot(up, up);
    if (upLengthSquared <= directionEpsilonSquared) {
        return;
    }
    up /= std::sqrt(upLengthSquared);

    glm::vec3 targetDirection;
    if (!TryGetTangentDirection(mTalkingNPC->GetPos() - mTalkingPlayer->GetPos(), up, targetDirection)) {
        return;
    }

    glm::vec3 currentDirection;
    if (!TryGetTangentDirection(mTalkingPlayer->GetFacingForwardVec(), up, currentDirection)) {
        currentDirection = targetDirection;
    }

    constexpr float talkFacingSmoothingSpeed = 7.0f;
    mTalkingPlayer->FaceDirection(
        RotateTowards(currentDirection, targetDirection, up, talkFacingSmoothingSpeed, deltaTime));
}

void TalkController::AdvanceTalk()
{
    mUIState->IncTalkUIIndex();
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void TalkController::StartTalkWithNPC(NPC* talkingNPC, Player* talkingPlayer)
{
    if (!talkingNPC || !talkingPlayer) {
        return;
    }

    mTalkingNPC = talkingNPC;
    mTalkingPlayer = talkingPlayer;

    mUIState->SetCurrentTalkWith(UIState::TalkWith::NPC);
    mUIState->SetTalkUIIndex(0);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void TalkController::TryStartTalkWithNPC(int playerNum)
{
    const std::vector<Player*>& players = mGame->GetPlayers();

    for (Player* player : players) {
        if (!player) {
            continue;
        }

        if (player->GetPlayerNum() != playerNum) {
            continue;
        }

        NPC* talkableNPC = player->GetTalkableNPC();
        if (!talkableNPC) {
            return;
        }

        if (!talkableNPC->GetIsTalkable()) {
            return;
        }

        StartTalkWithNPC(talkableNPC, player);
        return;
    }
}
