#include "gfx/UIRenderer.h"

#include "actor/OperationGuideZone.h"
#include "actor/Planet.h"
#include "actor/Player.h"
#include "app/Game.h"
#include "gfx/ui/HudRenderer.h"
#include "system/CameraSystem.h"
#include "system/SceneSystem.h"
#include "system/sequence/SequenceSystem.h"
#include "world/Stage.h"

#include <algorithm>

namespace {
constexpr float operationGuideRowSpacingRatio = 0.035f;
constexpr float operationGuideLeftRatio = 0.56f;
const UILoadSystem::CustomElement* FindOperationElement(const UILoadSystem& definitions, const std::string& id)
{
    const auto& elements = definitions.GetCustomElements();
    const auto found = std::find_if(elements.begin(), elements.end(), [&](const auto& element) {
        return element.screen == "operation" && element.id == id;
    });
    return found == elements.end() ? nullptr : &*found;
}
} // namespace

bool UIRenderer::CanShowContextualOperationGuide(int minimumStageNumber) const
{
    const SceneSystem* scene = mGame->GetSceneSystem();
    const SequenceSystem* sequence = mGame->GetSequenceSystem();
    const CameraSystem* camera = mGame->GetCameraSystem();
    return mGame->GetCurrentStageNum() >= minimumStageNumber && !mGame->GetIsUGCMode() &&
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
            const CameraSystem* camera = mGame->GetCameraSystem();
            availability.canExplainBlockedCamera[player->GetPlayerNum() - 1] =
                camera && camera->HasActiveCameraZone(player->GetPlayerNum() - 1);
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
    const bool canShow = CanShowContextualOperationGuide(1);
    const auto availability = GetOperationGuideAvailability();
    for (const Player* player : mGame->GetPlayers()) {
        if (!player || player->GetPlayerNum() < 1 || player->GetPlayerNum() > 2) {
            continue;
        }
        auto& previousHp = mPreviousGuidePlayerHp[player->GetPlayerNum() - 1];
        // 共有HPの同期を、別Playerの被ダメージとして二重に案内しない。
        const bool isGuideTarget = mGame->GetIsPlayer2Joined() || player == mGame->GetControlledPlayer();
        if (canShow && isGuideTarget && previousHp && player->GetHp() < *previousHp &&
            availability.canRecover[player->GetPlayerNum() - 1]) {
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
    if (CanShowContextualOperationGuide(1) && player && GetOperationGuideAvailability().canSplit) {
        mContextualOperationGuide.RequestSplit(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyPlayerSplitBlocked()
{
    const Player* player = mGame->GetControlledPlayer();
    if (CanShowContextualOperationGuide(1) && player && GetOperationGuideAvailability().canExplainBlockedSplit) {
        mContextualOperationGuide.RequestSplitBlocked(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyPlayerMergeTooFar()
{
    const Player* player = mGame->GetControlledPlayer();
    if (CanShowContextualOperationGuide(1) && player && GetOperationGuideAvailability().canExplainMergeTooFar) {
        mContextualOperationGuide.RequestMergeTooFar(player->GetPlayerNum());
    }
}

void UIRenderer::NotifyInsufficientJewels(int playerNumber, int requiredJewels)
{
    if (!CanShowContextualOperationGuide(1) || playerNumber < 1 || playerNumber > 2 || requiredJewels <= 0) {
        return;
    }
    const auto availability = GetOperationGuideAvailability();
    if (availability.activePlayers[playerNumber - 1] && availability.jewelCounts[playerNumber - 1] < requiredJewels) {
        mContextualOperationGuide.RequestInsufficientJewels(playerNumber, requiredJewels);
    }
}

void UIRenderer::NotifyCameraControlBlocked(int playerNumber)
{
    if (!CanShowContextualOperationGuide(1) || playerNumber < 1 || playerNumber > 2) {
        return;
    }
    const auto availability = GetOperationGuideAvailability();
    if (availability.activePlayers[playerNumber - 1] && availability.canExplainBlockedCamera[playerNumber - 1]) {
        mContextualOperationGuide.RequestCameraControlBlocked(playerNumber);
    }
}

void UIRenderer::DrawOperationGuidePrompt(const std::string& iconId, const std::string& textId,
                                          const std::string& description, const Player* player, float anchorX,
                                          float centerY, float scale, float opacity, const std::string& alternateIconId,
                                          const std::string& iconSeparator, float textScaleRatio, bool leftAligned)
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
    text.textScaleRatio = textScaleRatio;
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
    const float left = leftAligned ? anchorX : anchorX - (iconsWidth + textGap + textWidth) * 0.5f;
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
        text.xRatio = (left + iconsWidth * 0.5f) / mFbWidth;
        text.yRatio = centerY / mFbWidth;
        DrawCustomElement(text, 0.0f, 1.0f, false, scale, player, opacity, &iconSeparator);
    }
    text.xRatio = (left + iconsWidth + textGap + textWidth * 0.5f) / mFbWidth;
    text.yRatio = centerY / mFbWidth;
    DrawCustomElement(text, 0.0f, 1.0f, false, scale, player, opacity, &description);
}

void UIRenderer::DrawFatigueRecoveryPrompt(const Player* player, float screenTopY, float screenHeight)
{
    if (!mUILoadSystem || !player) {
        return;
    }
    const std::string textId =
        UsesControllerUI(player) ? "recommendReduceTiredTextForGameController" : "recommendReduceTiredTextForKeyBoard";
    const auto* text = mUILoadSystem->GetTextInfo("state", textId);
    if (!text || text->texts.empty()) {
        return;
    }

    DrawOperationGuidePrompt("buttonY", "buttonTextB_copy2_copy2_copy2", text->texts.front(), player,
                             mFbWidth * text->xRatio, screenTopY + screenHeight * text->yRatio, 1.0f, 1.0f, {}, "/",
                             text->scaleRatio);
}

void UIRenderer::DrawContextualOperationGuide()
{
    if (!CanShowContextualOperationGuide(0)) {
        return;
    }
    std::array<int, 2> usedRows{};
    for (const auto& prompt : mContextualOperationGuide.GetPrompts()) {
        if (prompt.playerNumber < 1 || prompt.playerNumber > 2) {
            continue;
        }
        int& row = usedRows[prompt.playerNumber - 1];
        if (row < ContextualOperationGuide::MaximumPromptsPerPlayer && DrawContextualOperationPrompt(prompt, row)) {
            ++row;
        }
    }
    DrawOperationGuideZones(usedRows);
}

bool UIRenderer::DrawContextualOperationPrompt(const ContextualOperationGuide::Prompt& prompt, int row)
{
    const Player* player = mGame->GetControlledPlayer();
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined();
    if (isTwoPlayer) {
        player = nullptr;
        for (const Player* candidate : mGame->GetPlayers()) {
            if (candidate && candidate->GetPlayerNum() == prompt.playerNumber) {
                player = candidate;
                break;
            }
        }
    }
    if (!player || player->GetPlayerNum() != prompt.playerNumber || !player->GetIsActive() || !player->IsAlive()) {
        return false;
    }
    const auto availability = GetOperationGuideAvailability();
    std::string iconId;
    std::string textId;
    std::string description;
    std::string alternateIconId;
    switch (prompt.kind) {
    case ContextualOperationGuide::Kind::SplitBlocked:
        if (!availability.canExplainBlockedSplit) {
            return false;
        }
        textId = "buttonTextB_copy2_copy2_copy";
        description = "まわりがせまくて分裂できないよ！";
        break;
    case ContextualOperationGuide::Kind::MergeTooFar:
        if (!availability.canExplainMergeTooFar) {
            return false;
        }
        textId = "buttonTextB_copy2_copy2_copy";
        description = "もう1体のスライムに近づこう！";
        break;
    case ContextualOperationGuide::Kind::InsufficientJewels:
        if (player->GetJewelCount() >= prompt.requiredJewels) {
            return false;
        }
        textId = "buttonTextB_copy2_copy2_copy2";
        description = "ジュエルが足りないよ！";
        break;
    case ContextualOperationGuide::Kind::CameraControlBlocked:
        if (!availability.canExplainBlockedCamera[player->GetPlayerNum() - 1]) {
            return false;
        }
        textId = "buttonTextB_copy2_copy2_copy";
        description = "ここではカメラを回転できません";
        break;
    case ContextualOperationGuide::Kind::Recover:
        if (!availability.canRecover[player->GetPlayerNum() - 1]) {
            return false;
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
            return false;
        }
        iconId = "buttonB_copy_copy2_copy";
        textId = "buttonTextB_copy2_copy2_copy";
        description = "分裂";
        break;
    }
    const float viewportHeight = mFbHeight * (isTwoPlayer ? 0.5f : 1.0f);
    const float viewportTop = isTwoPlayer && prompt.playerNumber == 2 ? viewportHeight : 0.0f;
    if (isTwoPlayer) {
        description = "P" + std::to_string(prompt.playerNumber) + "  " + description;
    }
    DrawOperationGuidePrompt(iconId, textId, description, player, mFbWidth * operationGuideLeftRatio,
                             viewportTop + viewportHeight * (0.84f - row * operationGuideRowSpacingRatio),
                             isTwoPlayer ? 0.52f : 0.65f, prompt.opacity, alternateIconId, "/", 0.00024f, true);
    return true;
}

void UIRenderer::DrawOperationGuideZones(std::array<int, 2> usedRows)
{
    const Stage* stage = mGame->GetCurrentStage();
    if (!stage) {
        return;
    }
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined();
    for (const Player* player : mGame->GetPlayers()) {
        if (!player || player->GetPlayerNum() < 1 || player->GetPlayerNum() > 2 || !player->GetIsActive() ||
            !player->IsAlive() || (!isTwoPlayer && player != mGame->GetControlledPlayer())) {
            continue;
        }
        int& row = usedRows[player->GetPlayerNum() - 1];
        if (row >= ContextualOperationGuide::MaximumPromptsPerPlayer) {
            continue;
        }
        std::vector<const OperationGuideZone*> zones;
        for (const Planet* planet : stage->GetPlanets()) {
            if (!planet) {
                continue;
            }
            for (const StageObject* actor : planet->GetStageObjects()) {
                const auto* zone = dynamic_cast<const OperationGuideZone*>(actor);
                if (zone && zone->GetIsActive() && zone->enabled && zone->Contains(player->GetPos())) {
                    zones.push_back(zone);
                }
            }
        }
        std::stable_sort(zones.begin(), zones.end(), [](const auto* left, const auto* right) {
            return left->priority != right->priority ? left->priority > right->priority
                                                     : left->GetStageYamlIndex() < right->GetStageYamlIndex();
        });
        std::vector<const OperationGuideZone::Operation*> displayed;
        for (const auto* zone : zones) {
            for (const auto& operation : zone->operations) {
                if (row >= ContextualOperationGuide::MaximumPromptsPerPlayer) {
                    break;
                }
                const bool duplicate = std::any_of(displayed.begin(), displayed.end(), [&](const auto* previous) {
                    return previous->iconId == operation.iconId && previous->description == operation.description &&
                           previous->repeatInput == operation.repeatInput;
                });
                if (operation.description.empty() || duplicate) {
                    continue;
                }
                std::string description = operation.description;
                if (isTwoPlayer) {
                    description = "P" + std::to_string(player->GetPlayerNum()) + "  " + description;
                }
                const float viewportHeight = mFbHeight * (isTwoPlayer ? 0.5f : 1.0f);
                const float viewportTop = isTwoPlayer && player->GetPlayerNum() == 2 ? viewportHeight : 0.0f;
                float scale = isTwoPlayer ? 0.52f : 0.65f;
                int textWidth = 0;
                int textHeight = 0;
                MeasureText(description, mFbWidth * 0.00024f * scale, textWidth, textHeight);
                if (textWidth > mFbWidth * 0.55f) {
                    scale *= mFbWidth * 0.55f / textWidth;
                }
                DrawOperationGuidePrompt(operation.iconId, "buttonTextB_copy2_copy2_copy", description, player,
                                         mFbWidth * operationGuideLeftRatio,
                                         viewportTop + viewportHeight * (0.84f - row * operationGuideRowSpacingRatio),
                                         scale, 1.0f, operation.repeatInput ? operation.iconId : "", "→", 0.00024f,
                                         true);
                displayed.push_back(&operation);
                ++row;
            }
        }
    }
}
