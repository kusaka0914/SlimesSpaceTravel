#include "system/scene/ArrivalSceneFlow.h"

#include "Game.h"
#include "Stage.h"
#include "actor/Boat.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "system/SceneSystem.h"
#include "system/scene/TalkController.h"
#include "system/scene/TutorialController.h"
#include "system/sequence/SequenceSystem.h"

ArrivalSceneFlow::ArrivalSceneFlow(
    Game& game,
    SceneSystem& sceneSystem,
    TalkController& talkController,
    TutorialController& tutorialController)
    : mGame(game),
      mSceneSystem(sceneSystem),
      mTalkController(talkController),
      mTutorialController(tutorialController)
{
}

void ArrivalSceneFlow::Update()
{
    UpdateTutorials();
    UpdateForcedTalk();
}

void ArrivalSceneFlow::Reset()
{
    mHasPendingForcedTalk = false;
    mHasReachedDestination = false;
    mHasPendingTutorials = false;
    mShouldSuppressForcedTalkOnce = false;
}

void ArrivalSceneFlow::PreparePlayingScene()
{
    mHasPendingForcedTalk = !mShouldSuppressForcedTalkOnce;
    mShouldSuppressForcedTalkOnce = false;
    mHasReachedDestination = false;
}

void ArrivalSceneFlow::PrepareStageChange()
{
    mHasPendingForcedTalk = true;
    mHasReachedDestination = false;
    mShouldSuppressForcedTalkOnce = false;
}

void ArrivalSceneFlow::SuppressForcedTalkOnce()
{
    mShouldSuppressForcedTalkOnce = true;
}

void ArrivalSceneFlow::OnBoatArrived(Boat* boat)
{
    if (!mGame.GetCurrentStage()) {
        return;
    }

    Player* mainPlayer = mGame.GetMainPlayer();
    for (Player* player : mGame.GetPlayers()) {
        if (!player) {
            continue;
        }
        const bool isInactiveSoloClone =
            !mGame.GetIsPlayer2Joined() &&
            !mGame.GetIsPlayerSplit() &&
            player != mainPlayer;
        if (!isInactiveSoloClone) {
            player->OnBoatArrived(boat);
        }
    }

    mHasPendingForcedTalk = true;
    mHasReachedDestination = true;
    mHasPendingTutorials = true;
}

void ArrivalSceneFlow::UpdateTutorials()
{
    if (!mHasPendingTutorials || !mSceneSystem.IsPlaying()) {
        return;
    }
    Player* controlledPlayer = mGame.GetControlledPlayer();
    if (!controlledPlayer || !controlledPlayer->GetIsActive() ||
        !controlledPlayer->GetOnGround()) {
        return;
    }

    mHasPendingTutorials = false;
    mTutorialController.TryStartBattleTutorial();
    mTutorialController.TryStartJustDodgeTutorial();
}

void ArrivalSceneFlow::UpdateForcedTalk()
{
    if (!mHasPendingForcedTalk || !mSceneSystem.IsPlaying() ||
        mSceneSystem.GetFadeTimer() >= 0.0f ||
        mSceneSystem.IsFadingOut()) {
        return;
    }
    SequenceSystem* sequenceSystem = mGame.GetSequenceSystem();
    if (sequenceSystem && sequenceSystem->IsPlaying()) {
        return;
    }

    Player* talkingPlayer = mGame.GetControlledPlayer();
    if (!talkingPlayer || !talkingPlayer->GetIsActive() ||
        (mHasReachedDestination && !talkingPlayer->GetOnGround())) {
        return;
    }

    mHasPendingForcedTalk = false;
    mHasReachedDestination = false;
    if (NPC* arrivalNPC = FindForcedTalkNPC()) {
        mTalkController.StartTalkWithNPC(arrivalNPC, talkingPlayer);
    }
}

NPC* ArrivalSceneFlow::FindForcedTalkNPC() const
{
    Player* controlledPlayer = mGame.GetControlledPlayer();
    Planet* arrivalPlanet =
        controlledPlayer ? controlledPlayer->GetCurrentPlanet() : nullptr;
    const auto findInPlanet = [this](Planet* planet) -> NPC* {
        if (!planet) {
            return nullptr;
        }
        for (NPC* npc : planet->GetNPCs()) {
            if (npc && npc->GetIsActive() &&
                npc->GetForcesTalkOnArrival() &&
                !npc->GetResolvedTalkTexts().empty() &&
                !mGame.HasShownNPCConversation(npc)) {
                return npc;
            }
        }
        return nullptr;
    };

    if (NPC* arrivalNPC = findInPlanet(arrivalPlanet)) {
        return arrivalNPC;
    }
    Stage* stage = mGame.GetCurrentStage();
    if (stage) {
        for (Planet* planet : stage->GetPlanets()) {
            if (planet != arrivalPlanet) {
                if (NPC* arrivalNPC = findInPlanet(planet)) {
                    return arrivalNPC;
                }
            }
        }
    }
    return nullptr;
}
