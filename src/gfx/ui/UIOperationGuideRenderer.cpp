#include "gfx/UIRenderer.h"

#include "actor/Player.h"
#include "app/Game.h"
#include "gfx/ui/HudRenderer.h"
#include "system/CameraSystem.h"
#include "system/SceneSystem.h"
#include "system/sequence/SequenceSystem.h"

#include <algorithm>

namespace {
const UILoadSystem::CustomElement* FindOperationElement(const UILoadSystem& definitions, const std::string& id)
{
    const auto& elements = definitions.GetCustomElements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) {
        return element.screen == "operation" && element.id == id;
    });
    return found == elements.end() ? nullptr : &*found;
}
} // namespace

bool UIRenderer::CanShowContextualOperationGuide(bool allowsStageOne) const
{
    const SceneSystem* scene = mGame->GetSceneSystem();
    const SequenceSystem* sequence = mGame->GetSequenceSystem();
    const CameraSystem* camera = mGame->GetCameraSystem();
    return mGame->GetCurrentStageNum() >= (allowsStageOne ? 1 : 2) && !mGame->GetIsUGCMode() &&
           !mGame->GetIsUGCPlaytestActive() && !mGame->GetIsPauseMenuOpen() && !mGame->GetIsDebugEditorShowing() &&
           !mGame->GetIsGameUIHidden() && !mGame->GetIsFreeCameraMode() && scene && scene->IsPlaying() &&
           !scene->HasActiveTutorial() && !scene->IsTalkWithNPC() && !scene->IsTransitionActive() &&
           (!sequence || (!sequence->IsCinematicChainPlaying() && !sequence->LocksPlayerControl())) && camera &&
           camera->AllowsPlayerInput();
}

ContextualOperationGuide::Availability UIRenderer::GetOperationGuideAvailability() const
{
    ContextualOperationGuide::Availability availability;
    for (const Player* player : mGame->GetPlayers()) {
        if (player && player->GetPlayerNum() >= 1 && player->GetPlayerNum() <= 2) {
            availability.activePlayers[player->GetPlayerNum() - 1] = player->GetIsActive() && player->IsAlive();
            availability.jewelCounts[player->GetPlayerNum() - 1] = player->GetJewelCount();
            availability.canRecover[player->GetPlayerNum() - 1] = player->GetIsActive() && player->IsAlive() &&
                                                                  player->GetHp() < player->GetMaxHp() &&
                                                                  player->GetJewelCount() >= 1;
        }
    }
    const bool isSolo = !mGame->GetIsPlayer2Joined();
    availability.canSplit = isSolo && !mGame->GetIsPlayerSplit() && mGame->CanTogglePlayerSplit();
    const Player* controlledPlayer = mGame->GetControlledPlayer();
    availability.canExplainBlockedSplit = isSolo && !mGame->GetIsPlayerSplit() && controlledPlayer &&
                                          controlledPlayer->GetIsActive() && controlledPlayer->IsAlive();
    availability.canExplainMergeTooFar =
        isSolo && mGame->GetIsPlayerSplit() && mGame->CanSwitchControlledPlayer() && !mGame->CanTogglePlayerSplit();
    return availability;
}

void UIRenderer::UpdateOperationGuides(float deltaTime)
{
    if (mOperationGuideStageNumber != mGame->GetCurrentStageNum()) {
        ResetOperationGuides();
    }
    if (mOperationGuideWasTwoPlayer != mGame->GetIsPlayer2Joined()) {
        mContextualOperationGuide.Update(0.0f, false, {});
        mPreviousGuidePlayerHp.fill(std::nullopt);
        mOperationGuideWasTwoPlayer = mGame->GetIsPlayer2Joined();
    }
    const bool canShow = CanShowContextualOperationGuide(true);
    const auto availability = GetOperationGuideAvailability();
    for (const Player* player : mGame->GetPlayers()) {
        if (!player || player->GetPlayerNum() < 1 || player->GetPlayerNum() > 2) {
            continue;
        }
        auto& previousHp = mPreviousGuidePlayerHp[player->GetPlayerNum() - 1];
        // 共有HPの同期を、別Playerの被ダメージとして二重に案内しない。
        const bool isGuideTarget = mGame->GetIsPlayer2Joined() || player == mGame->GetControlledPlayer();
        if (canShow && mGame->GetCurrentStageNum() >= 2 && isGuideTarget && previousHp &&
            player->GetHp() < *previousHp && availability.canRecover[player->GetPlayerNum() - 1]) {
            const bool hasEnteredCriticalHealth = *previousHp > 1.0f && player->GetHp() <= 1.0f;
            mContextualOperationGuide.RequestRecovery(player->GetPlayerNum(), hasEnteredCriticalHealth);
        }
        previousHp = player->GetHp();
    }
    mContextualOperationGuide.Update(deltaTime, canShow, availability);
}

void UIRenderer::ResetOperationGuides()
{
    if (mHudRenderer) {
        mHudRenderer->ResetConsumptionFeedback();
    }
    mContextualOperationGuide.Reset();
    mPreviousGuidePlayerHp.fill(std::nullopt);
    mOperationGuideStageNumber = mGame->GetCurrentStageNum();
    mOperationGuideWasTwoPlayer = mGame->GetIsPlayer2Joined();
}

void UIRenderer::NotifyPlayerRecovered(int playerNumber)
{
    // ソロ分身ではHP・ジュエルが共有されるため、どちらで回復しても回復案内を閉じる。
    mContextualOperationGuide.NotifyRecovered(mGame->GetIsPlayer2Joined() ? playerNumber : 0);
}

void UIRenderer::NotifyPlayerSwitchRequested(bool didSwitch)
{
    if (didSwitch) {
        return;
    }
    const Player* player = mGame->GetControlledPlayer();
    if (CanShowContextualOperationGuide() && player && GetOperationGuideAvailability().canSplit) {
        mContextualOperationGuide.RequestSplit(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyPlayerSplitBlocked()
{
    const Player* player = mGame->GetControlledPlayer();
    if (CanShowContextualOperationGuide(true) && player && GetOperationGuideAvailability().canExplainBlockedSplit) {
        mContextualOperationGuide.RequestSplitBlocked(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyPlayerMergeTooFar()
{
    const Player* player = mGame->GetControlledPlayer();
    if (CanShowContextualOperationGuide(true) && player && GetOperationGuideAvailability().canExplainMergeTooFar) {
        mContextualOperationGuide.RequestMergeTooFar(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyInsufficientJewels(int playerNumber, int requiredJewels)
{
    if (!CanShowContextualOperationGuide(true) || playerNumber < 1 || playerNumber > 2 || requiredJewels <= 0) {
        return;
    }
    const auto availability = GetOperationGuideAvailability();
    if (availability.activePlayers[playerNumber - 1] && availability.jewelCounts[playerNumber - 1] < requiredJewels) {
        mContextualOperationGuide.RequestInsufficientJewels(playerNumber, requiredJewels);
    }
}

void UIRenderer::DrawOperationGuidePrompt(const std::string& iconId, const std::string& textId,
                                          const std::string& description, const Player* player, float centerX,
                                          float centerY, float scale, float opacity, const std::string& alternateIconId)
{
    if (!mUILoadSystem || mFbWidth <= 0 || opacity <= 0.0f) {
        return;
    }
    const auto* iconSource = FindOperationElement(*mUILoadSystem, iconId);
    const auto* textSource = FindOperationElement(*mUILoadSystem, textId);
    if ((!iconId.empty() && !iconSource) || !textSource) {
        return;
    }
    auto text = *textSource;
    text.centerBased = true;
    text.textScaleRatio = 0.00024f;
    text.rotationDegrees = 0.0f;
    text.outlineEnabled = true;
    text.outlineWidthRatio = std::max(0.001f, text.outlineWidthRatio);
    text.outlineColor = {0.0f, 0.0f, 0.0f, 1.0f};
    int textWidth = 0;
    int textHeight = 0;
    MeasureText(description, mFbWidth * text.textScaleRatio * scale, textWidth, textHeight);
    const float iconSize = mFbWidth * 0.025f * scale;
    const float gap = mFbWidth * 0.007f * scale;
    const bool hasAlternate = !alternateIconId.empty();
    const float iconsWidth = !iconSource ? 0.0f : (hasAlternate ? iconSize * 2.0f + gap * 3.0f : iconSize);
    const float textGap = iconSource ? gap : 0.0f;
    const float left = centerX - (iconsWidth + textGap + textWidth) * 0.5f;
    const auto drawIcon = [&](const UILoadSystem::CustomElement& source, float x) {
        auto icon = source;
        icon.centerBased = true;
        icon.xRatio = x / mFbWidth;
        icon.yRatio = centerY / mFbWidth;
        icon.widthRatio = icon.heightRatio = 0.025f;
        icon.rotationDegrees = 0.0f;
        DrawCustomElement(icon, 0.0f, 1.0f, false, scale, player, opacity);
    };
    if (iconSource) {
        drawIcon(*iconSource, left + iconSize * 0.5f);
    }
    if (hasAlternate) {
        const auto* alternate = FindOperationElement(*mUILoadSystem, alternateIconId);
        if (alternate) {
            drawIcon(*alternate, left + iconsWidth - iconSize * 0.5f);
        }
        const std::string separator = "/";
        text.xRatio = (left + iconsWidth * 0.5f) / mFbWidth;
        text.yRatio = centerY / mFbWidth;
        DrawCustomElement(text, 0.0f, 1.0f, false, scale, player, opacity, &separator);
    }
    text.xRatio = (left + iconsWidth + textGap + textWidth * 0.5f) / mFbWidth;
    text.yRatio = centerY / mFbWidth;
    DrawCustomElement(text, 0.0f, 1.0f, false, scale, player, opacity, &description);
}

void UIRenderer::DrawContextualOperationGuide()
{
    if (!CanShowContextualOperationGuide(true)) {
        return;
    }
    const auto prompt = mContextualOperationGuide.GetPrompt();
    if (!prompt) {
        return;
    }
    const Player* player = mGame->GetControlledPlayer();
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined();
    if (isTwoPlayer) {
        player = nullptr;
        for (const Player* candidate : mGame->GetPlayers()) {
            if (candidate && candidate->GetPlayerNum() == prompt->playerNumber) {
                player = candidate;
                break;
            }
        }
    }
    if (!player || player->GetPlayerNum() < 1 || player->GetPlayerNum() > 2 || !player->GetIsActive() ||
        !player->IsAlive()) {
        return;
    }
    const auto availability = GetOperationGuideAvailability();
    std::string iconId;
    std::string textId;
    std::string description;
    std::string alternateIconId;
    switch (prompt->kind) {
    case ContextualOperationGuide::Kind::SplitBlocked:
        if (!availability.canExplainBlockedSplit) {
            return;
        }
        textId = "buttonTextB_copy2_copy2_copy";
        description = "まわりがせまくて分裂できないよ！";
        break;
    case ContextualOperationGuide::Kind::MergeTooFar:
        if (!availability.canExplainMergeTooFar) {
            return;
        }
        textId = "buttonTextB_copy2_copy2_copy";
        description = "もう1体のスライムに近づこう！";
        break;
    case ContextualOperationGuide::Kind::InsufficientJewels:
        if (player->GetJewelCount() >= prompt->requiredJewels) {
            return;
        }
        textId = "buttonTextB_copy2_copy2_copy2";
        description = "ジュエルが足りないよ！";
        break;
    case ContextualOperationGuide::Kind::Recover:
        if (!availability.canRecover[player->GetPlayerNum() - 1]) {
            return;
        }
        iconId = "buttonB_copy_copy2_copy2";
        textId = "buttonTextB_copy2_copy2_copy2";
        description = "回復";
        if (mGame->IsAssistControlStyle()) {
            alternateIconId = "buttonX";
        }
        break;
    case ContextualOperationGuide::Kind::Split:
        if (!availability.canSplit) {
            return;
        }
        iconId = "buttonB_copy_copy2_copy";
        textId = "buttonTextB_copy2_copy2_copy";
        description = "分裂";
        break;
    }
    const float viewportHeight = mFbHeight * (isTwoPlayer ? 0.5f : 1.0f);
    const float viewportTop = isTwoPlayer && prompt->playerNumber == 2 ? viewportHeight : 0.0f;
    if (isTwoPlayer) {
        description = "P" + std::to_string(prompt->playerNumber) + "  " + description;
    }
    DrawOperationGuidePrompt(iconId, textId, description, player, mFbWidth * 0.61f,
                             viewportTop + viewportHeight * 0.84f, isTwoPlayer ? 0.52f : 0.65f, prompt->opacity,
                             alternateIconId);
}
