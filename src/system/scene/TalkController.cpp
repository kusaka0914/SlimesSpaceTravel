#include "system/scene/TalkController.h"

#include "Game.h"
#include "actor/NPC.h"
#include "actor/Player.h"
#include "state/GameProgressState.h"
#include "state/UIState.h"
#include "system/AudioSystem.h"
#include "system/InputSystem.h"
#include "system/SceneSystem.h"

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
}

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
    if (FinishTalkIfComplete()) {
        return;
    }

    if (mGameProgressState->GetSceneState() != GameProgressState::SceneState::Talking ||
        mUIState->GetCurrentTalkWith() != UIState::TalkWith::NPC ||
        !mTalkingNPC || !mTalkingPlayer ||
        !mTalkingNPC->GetIsActive() || !mTalkingPlayer->GetIsActive()) {
        return;
    }

    if (TryAdvanceTalkFromCompletedAction()) {
        return;
    }

    if (!mTalkingNPC->ShouldFacePlayerDuringTalk()) {
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

bool TalkController::FinishTalkIfComplete()
{
    if (!mGameProgressState || !mUIState ||
        mGameProgressState->GetSceneState() !=
            GameProgressState::SceneState::Talking ||
        mUIState->GetCurrentTalkWith() != UIState::TalkWith::NPC) {
        return false;
    }

    const int talkPageIndex = mUIState->GetTalkUIIndex();
    const bool hasRemainingPage =
        mTalkingNPC && talkPageIndex >= 0 &&
        talkPageIndex <
            static_cast<int>(mTalkingNPC->GetResolvedTalkTexts().size());
    if (hasRemainingPage) {
        return false;
    }

    if (mTalkingNPC) {
        mTalkingNPC->MarkTalkCompletedThisVisit();
    }
    mGame->StartPlayingScene();
    mUIState->FinishTalkWith();
    return true;
}

void TalkController::TryAdvanceTalkFromConfirm()
{
    if (GetCurrentAdvanceCondition() !=
        TalkPageAdvanceCondition::Confirm) {
        return;
    }

    AdvanceTalkPage();
}

void TalkController::AdvanceTalkPage()
{
    const int talkPageIndex = mUIState->GetTalkUIIndex();
    const std::optional<std::size_t> sourceTalkPageIndex =
        mTalkingNPC && talkPageIndex >= 0
            ? mTalkingNPC->GetResolvedTalkSourceIndex(
                  static_cast<std::size_t>(talkPageIndex))
            : std::nullopt;
    const bool startsOpening =
        mTalkingNPC && talkPageIndex >= 0 &&
        mTalkingNPC->GetResolvedTalkStartsOpeningAfterPage(
            static_cast<std::size_t>(talkPageIndex));
    const bool startsEnding =
        mTalkingNPC && talkPageIndex >= 0 &&
        mTalkingNPC->GetResolvedTalkStartsEndingAfterPage(
            static_cast<std::size_t>(talkPageIndex));

    mUIState->IncTalkUIIndex();
    CaptureCurrentPageActionBaseline();
    mGame->GetAudioSystem()->PlaySE("message_se");

    if (startsEnding && sourceTalkPageIndex &&
        mGame->GetSceneSystem()->StartEndingAfterTalkPage(
            mTalkingNPC, *sourceTalkPageIndex)) {
        return;
    }

    if (startsOpening && sourceTalkPageIndex) {
        mGame->GetSceneSystem()->StartOpeningAfterTalkPage(
            mTalkingNPC, mTalkingPlayer, talkPageIndex + 1,
            *sourceTalkPageIndex);
    }
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
    mGame->MarkNPCConversationShown(talkingNPC);
    CaptureCurrentPageActionBaseline();
    mGame->GetAudioSystem()->PlaySE("message_se");
}

void TalkController::ResumeTalkWithNPC(
    NPC* talkingNPC, Player* talkingPlayer, int talkPageIndex)
{
    if (!talkingNPC || !talkingPlayer || talkPageIndex < 0) {
        return;
    }

    mTalkingNPC = talkingNPC;
    mTalkingPlayer = talkingPlayer;
    mUIState->SetCurrentTalkWith(UIState::TalkWith::NPC);
    mUIState->SetTalkUIIndex(talkPageIndex);
    mGameProgressState->SetCurrentSceneState(GameProgressState::SceneState::Talking);
    CaptureCurrentPageActionBaseline();
}

TalkPageAdvanceCondition
TalkController::GetCurrentAdvanceCondition() const
{
    if (!mTalkingNPC || !mUIState) {
        return TalkPageAdvanceCondition::Confirm;
    }

    const int talkPageIndex = mUIState->GetTalkUIIndex();
    if (talkPageIndex < 0) {
        return TalkPageAdvanceCondition::Confirm;
    }

    return mTalkingNPC->GetResolvedTalkAdvanceCondition(
        static_cast<std::size_t>(talkPageIndex));
}

void TalkController::CaptureCurrentPageActionBaseline()
{
    mActionPlayerAtPageStart =
        mGame ? mGame->GetControlledPlayer() : nullptr;
    mControlledPlayerIndexAtPageStart =
        mGame ? mGame->GetControlledPlayerIndex() : -1;
    mJumpSequenceAtPageStart =
        mActionPlayerAtPageStart
            ? mActionPlayerAtPageStart->GetJumpSequence()
            : 0;
    mHasJumpStartedOnCurrentPage = false;
}

bool TalkController::TryAdvanceTalkFromCompletedAction()
{
    switch (GetCurrentAdvanceCondition()) {
    case TalkPageAdvanceCondition::PlayerSwitch:
        if (!mGame ||
            mGame->GetControlledPlayerIndex() ==
                mControlledPlayerIndexAtPageStart) {
            return false;
        }
        mTalkingPlayer = mGame->GetControlledPlayer();
        AdvanceTalkPage();
        return true;

    case TalkPageAdvanceCondition::Jump: {
        Player* controlledPlayer =
            mGame ? mGame->GetControlledPlayer() : nullptr;
        if (!controlledPlayer ||
            controlledPlayer != mActionPlayerAtPageStart) {
            return false;
        }

        if (controlledPlayer->GetJumpSequence() >
            mJumpSequenceAtPageStart) {
            mHasJumpStartedOnCurrentPage = true;
        }
        if (!mHasJumpStartedOnCurrentPage ||
            !controlledPlayer->GetOnGround()) {
            return false;
        }

        AdvanceTalkPage();
        return true;
    }

    case TalkPageAdvanceCondition::Confirm:
    default:
        return false;
    }
}

bool TalkController::IsWaitingForPlayerAction() const
{
    if (!mGameProgressState || !mUIState || !mTalkingNPC ||
        mGameProgressState->GetSceneState() !=
            GameProgressState::SceneState::Talking ||
        mUIState->GetCurrentTalkWith() != UIState::TalkWith::NPC) {
        return false;
    }

    return GetCurrentAdvanceCondition() !=
           TalkPageAdvanceCondition::Confirm;
}

bool TalkController::IsWaitingForPlayerSwitch() const
{
    return IsWaitingForPlayerAction() &&
           GetCurrentAdvanceCondition() ==
               TalkPageAdvanceCondition::PlayerSwitch;
}

bool TalkController::IsWaitingForPlayerJump() const
{
    return IsWaitingForPlayerAction() &&
           GetCurrentAdvanceCondition() ==
               TalkPageAdvanceCondition::Jump;
}

bool TalkController::TryStartTalkWithNPC(int playerNum)
{
    const std::vector<Player*>& players = mGame->GetPlayers();

    for (Player* player : players) {
        if (!player) {
            continue;
        }

        if (player->GetPlayerNum() != playerNum) {
            continue;
        }

        if (!CanStartTalkWithNPC(player)) {
            return false;
        }

        StartTalkWithNPC(player->GetTalkableNPC(), player);
        return true;
    }

    return false;
}

bool TalkController::CanStartTalkWithNPC(
    const Player* player) const
{
    if (!player) {
        return false;
    }

    NPC* talkableNPC = player->GetTalkableNPC();
    if (!talkableNPC ||
        !talkableNPC->GetIsTalkable() ||
        !talkableNPC->CanStartRegularTalk()) {
        return false;
    }

    const InputSystem* inputSystem = mGame->GetInputSystem();
    return !inputSystem ||
           !inputSystem->IsMovementInputPressedForPlayer(player);
}
