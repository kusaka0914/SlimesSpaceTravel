#include "gfx/UIRenderer.h"

#include "actor/Player.h"
#include "app/Game.h"
#include "gfx/ui/OperationGuideVisibility.h"
#include "imgui.h"
#include "system/CameraSystem.h"
#include "system/SceneSystem.h"

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <unordered_map>

namespace {

bool IsBuiltInUGCEditorElement(const std::string& id)
{
    return id == "presetTools" || id == "selection" || id == "menu" || id == "quickTools" || id == "keyboardTools" ||
           id == "play" || id == "preview" || id == "presetPlatform" || id == "presetEnemy" || id == "presetPlanet" ||
           id == "presetSwitch" || id == "presetGoal" || id == "eraser" || id == "undo" || id == "redo" ||
           id == "layerUp" || id == "layerDown" || id == "zoomIn" || id == "zoomOut";
}

} // namespace

void UIRenderer::DrawCustomElement(const UILoadSystem::CustomElement& element, float viewportTopY, float viewportScale,
                                   bool centerTalkPrompt, float contentScale, const Player* inputPlayer, float opacity,
                                   const std::string* textOverride)
{
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (contentScale < 0.0f) {
        contentScale = viewportScale;
    }
    float x = mFbWidth * element.xRatio;
    float y = viewportTopY + mFbWidth * element.yRatio * viewportScale;
    const float width = mFbWidth * element.widthRatio * contentScale;
    const float height = mFbWidth * element.heightRatio * contentScale;

    if (centerTalkPrompt) {
        float centerX = static_cast<float>(mFbWidth) * 0.5f;
        float centerY = viewportTopY + static_cast<float>(mFbHeight) * 0.25f;
        float iconCenterX = centerX;
        float iconCenterY = centerY;
        float textCenterX = centerX;
        float textCenterY = centerY;
        for (const UILoadSystem::CustomElement& candidate : mUILoadSystem->GetCustomElements()) {
            if (candidate.id == "multiplayerTalkIconAnchor") {
                iconCenterX = mFbWidth * candidate.xRatio;
                iconCenterY = viewportTopY + mFbWidth * candidate.yRatio;
            } else if (candidate.id == "multiplayerTalkTextAnchor") {
                textCenterX = mFbWidth * candidate.xRatio;
                textCenterY = viewportTopY + mFbWidth * candidate.yRatio;
            }
        }
        if (element.type == UILoadSystem::CustomElementType::Image) {
            x = iconCenterX - width * 0.5f;
            y = iconCenterY - height * 0.5f;
        } else if (element.type == UILoadSystem::CustomElementType::Text) {
            x = textCenterX;
            y = textCenterY;
        }
    }

    const float topLeftX = element.centerBased ? x - width * 0.5f : x;
    const float topLeftY = element.centerBased ? y - height * 0.5f : y;

    switch (element.type) {
    case UILoadSystem::CustomElementType::Text: {
        std::string resolvedText = textOverride ? *textOverride : ResolveCustomElementText(element);
        if (inputPlayer && element.usesInputDeviceVariants && !textOverride) {
            const bool usesController = UsesControllerUI(inputPlayer);
            const std::string& deviceText = usesController ? element.gameControllerText : element.keyboardText;
            if (!deviceText.empty()) {
                resolvedText = deviceText;
            }
        }
        glm::vec4 textColor(element.color[0] * 255.0f, element.color[1] * 255.0f, element.color[2] * 255.0f,
                            element.color[3] * 255.0f * opacity);
        if (element.screen == "title") {
            constexpr std::array<const char*, 3> titleMenuIds = {"startGame", "createStage", "playCreatedStage"};
            for (int index = 0; index < static_cast<int>(titleMenuIds.size()); ++index) {
                if (element.id == titleMenuIds[index] && mGame->GetTitleMenuSelection() == index) {
                    textColor = glm::vec4(255.0f, 225.0f, 45.0f, 255.0f * opacity);
                    break;
                }
            }
        }
        TextEffect effect;
        effect.shadowEnabled = element.shadowEnabled;
        effect.shadowOffset = static_cast<float>(mFbWidth) * contentScale *
                              glm::vec2(element.shadowOffsetXRatio, element.shadowOffsetYRatio);
        effect.shadowColor = glm::vec4(element.shadowColor[0] * 255.0f, element.shadowColor[1] * 255.0f,
                                       element.shadowColor[2] * 255.0f, element.shadowColor[3] * 255.0f * opacity);
        effect.outlineEnabled = element.outlineEnabled;
        effect.outlineWidth = mFbWidth * element.outlineWidthRatio * contentScale;
        effect.outlineColor = glm::vec4(element.outlineColor[0] * 255.0f, element.outlineColor[1] * 255.0f,
                                        element.outlineColor[2] * 255.0f, element.outlineColor[3] * 255.0f * opacity);
        const float textScale = mFbWidth * element.textScaleRatio * contentScale;
        const std::vector<RubyTextSegment>& rubySegments = ResolveCustomElementRuby(resolvedText);
        const bool hasRuby = std::any_of(rubySegments.begin(), rubySegments.end(), [](const RubyTextSegment& segment) {
            return segment.showsRuby && !segment.reading.empty();
        });
        if (hasRuby) {
            constexpr float customRubyScaleRatio = 0.42f;
            constexpr float customRubyGapRatio = -0.12f;
            if (effect.shadowEnabled && effect.shadowColor.a > 0.0f) {
                DrawRubyText(x + effect.shadowOffset.x, y + effect.shadowOffset.y, textScale, customRubyScaleRatio,
                             customRubyGapRatio, rubySegments, effect.shadowColor, element.centerBased,
                             element.rotationDegrees);
            }
            DrawRubyText(x, y, textScale, customRubyScaleRatio, customRubyGapRatio, rubySegments, textColor,
                         element.centerBased, element.rotationDegrees,
                         effect.outlineEnabled ? effect.outlineWidth : 0.0f, effect.outlineColor);
        } else {
            DrawText(x, y, textScale, resolvedText, element.centerBased, textColor, element.rotationDegrees, &effect);
        }
        break;
    }
    case UILoadSystem::CustomElementType::Image: {
        std::string texturePath = ResolveCustomElementTexturePath(element);
        bool flipVertical = ResolveCustomElementTextureFlipVertical(element);
        if (inputPlayer && element.usesInputDeviceVariants) {
            const bool usesController = UsesControllerUI(inputPlayer);
            const std::string& deviceTexturePath =
                usesController ? element.gameControllerTexturePath : element.keyboardTexturePath;
            if (!deviceTexturePath.empty()) {
                texturePath = deviceTexturePath;
                flipVertical = usesController ? element.gameControllerFlipVertical : element.keyboardFlipVertical;
            }
        }
        if (RegisterCustomUITexture(texturePath)) {
            const auto textureIt = mTextures.find(GetCustomTextureName(texturePath));
            if (textureIt != mTextures.end()) {
                DrawTextureHandle(topLeftX, topLeftY, width, height, textureIt->second, flipVertical,
                                  element.rotationDegrees, opacity);
            }
        }
        break;
    }
    case UILoadSystem::CustomElementType::Panel:
        DrawBG(topLeftX, topLeftY, width, height,
               {element.color[0], element.color[1], element.color[2], element.color[3] * opacity},
               element.rotationDegrees);
        break;
    }
}

float UIRenderer::CalculateOperationGuideOpacity(const UILoadSystem::CustomElement& element, const Player* player,
                                                 std::optional<bool>& canTogglePlayerSplit) const
{
    constexpr float disabledOpacity = 0.38f;
    if (element.screen != "operation" ||
        (mGame->GetIsPauseMenuOpen() && !mGame->GetIsUGCMode() && !mGame->GetIsUGCPlaytestActive())) {
        return 1.0f;
    }

    const bool isBoatDisembarkGuide =
        player && player->IsWaitingForBoat() && (element.id == "buttonB" || element.id == "buttonTextB");
    if (isBoatDisembarkGuide) {
        return 1.0f;
    }

    const bool playerCanAct = player && player->GetIsActive() && player->IsAlive();
    if (!playerCanAct) {
        return disabledOpacity;
    }

    const bool isSpecialCharging = player->IsSpecialCharging() || player->GetCanSpecialAttack();
    const bool isContinuousAttack = player->IsContinuousAttacking();
    // 連続攻撃は地上にいる間だけ操作を占有する。空中では残り時間が
    // 減るだけで、弱攻撃・回避・強攻撃を通常どおり受け付ける。
    const bool isGroundContinuousAttack = isContinuousAttack && player->GetOnGround();
    const bool isIdle = player->GetActionState() == Player::ActionState::Idle;
    const bool isWeakAttacking = player->GetActionState() == Player::ActionState::Attacking;
    const bool canStartNormalAction = isIdle && !isSpecialCharging && !isGroundContinuousAttack;
    const bool canRecover = isIdle && !isSpecialCharging && !player->GetCanSpecialAttack() &&
                            player->GetJewelCount() >= 1 && player->GetHp() < player->GetMaxHp();
    const bool canStartChargeAttack =
        (isIdle || isWeakAttacking) && !isGroundContinuousAttack && player->GetJewelCount() >= 2;
    const bool canStartContinuousAttack = (isIdle || isWeakAttacking) && !isSpecialCharging &&
                                          !isGroundContinuousAttack && player->GetOnGround() &&
                                          player->GetJewelCount() >= PlayerJewelGauge::ContinuousAttackCost;
    bool isEnabled = true;

    if (element.id == "weakAttackHoldGuide") {
        isEnabled = canStartContinuousAttack || isGroundContinuousAttack;
    } else if (element.id == "strongAttackHoldGuide") {
        isEnabled = canStartChargeAttack || isSpecialCharging;
    } else if (element.id == "buttonA" || element.id == "buttonTextA") {
        isEnabled = player->GetOnGround()
                        ? (isIdle && !isSpecialCharging)
                        : (!isSpecialCharging && !isGroundContinuousAttack && (isIdle || isWeakAttacking));
    } else if (element.id == "buttonB" || element.id == "buttonTextB") {
        // 回避の短いクールタイムでは点滅させず、連続攻撃中など入力が封じられる状態だけ示す。
        // 溜め中は回避を受け付けない。
        isEnabled = !isSpecialCharging && !isGroundContinuousAttack && (isIdle || isWeakAttacking);
    } else if (element.id == "buttonA_copy" || element.id == "buttonTextB_copy") {
        isEnabled = mGame->CanOpenPauseMenu();
    } else if (element.id == "buttonX" || element.id == "buttonTextX") {
        isEnabled = isSpecialCharging || canStartChargeAttack ||
                    (mGame->IsAssistControlStyle() ? canRecover : canStartNormalAction);
    } else if (element.id == "buttonY" || element.id == "buttonTextY") {
        isEnabled = canStartNormalAction || canStartContinuousAttack;
    } else if (element.id == "buttonB_copy" || element.id == "buttonTextB_copy2") {
        isEnabled = isIdle;
    } else if (element.id == "buttonB_copy_copy" || element.id == "buttonTextB_copy2_copy" ||
               element.id == "buttonB_copy_copy_copy" || element.id == "buttonTextB_copy2_copy_copy") {
        const CameraSystem* cameraSystem = mGame->GetCameraSystem();
        isEnabled = cameraSystem && cameraSystem->AllowsPlayerInput();
    } else if (element.id == "buttonB_copy_copy2" || element.id == "buttonTextB_copy2_copy2") {
        isEnabled = mGame->CanSwitchControlledPlayer();
    } else if (element.id == "buttonB_copy_copy2_copy2" || element.id == "buttonTextB_copy2_copy2_copy2") {
        isEnabled = canRecover;
    } else if (element.id == "buttonB_copy_copy2_copy" || element.id == "buttonTextB_copy2_copy2_copy") {
        if (!canTogglePlayerSplit) {
            canTogglePlayerSplit = mGame->CanTogglePlayerSplit();
        }
        isEnabled = *canTogglePlayerSplit;
    }

    return isEnabled ? 1.0f : disabledOpacity;
}

// 画面ごとの配置を保ちながら、高さが足りない場合だけ縮小して枠内に収める。
UIRenderer::CustomScreenLayout
UIRenderer::CalculateCustomScreenLayout(const std::vector<const UILoadSystem::CustomElement*>& sortedElements,
                                        const std::string& screen, float viewportHeight, bool alwaysAnchorToBottom,
                                        float preferredVerticalOffset) const
{
    constexpr float viewportPadding = 12.0f;
    float minimumY = std::numeric_limits<float>::max();
    float maximumY = std::numeric_limits<float>::lowest();
    for (const UILoadSystem::CustomElement* candidate : sortedElements) {
        if (!candidate || candidate->screen != screen) {
            continue;
        }

        float height = mFbWidth * candidate->heightRatio;
        if (candidate->type == UILoadSystem::CustomElementType::Text) {
            const std::string resolvedText = ResolveCustomElementText(*candidate);
            std::string firstLine;
            std::string secondLine;
            const bool hasSecondLine = SplitText(resolvedText, firstLine, secondLine);
            int firstWidth = 0;
            int firstHeight = 0;
            MeasureText(firstLine, mFbWidth * candidate->textScaleRatio, firstWidth, firstHeight);
            height = static_cast<float>(firstHeight);
            if (hasSecondLine) {
                int secondWidth = 0;
                int secondHeight = 0;
                MeasureText(secondLine, mFbWidth * candidate->textScaleRatio, secondWidth, secondHeight);
                height += static_cast<float>(secondHeight) + mFbHeight * 0.0666f;
            }
        }

        const float y = mFbWidth * candidate->yRatio;
        const float top = candidate->centerBased ? y - height * 0.5f : y;
        minimumY = std::min(minimumY, top);
        maximumY = std::max(maximumY, top + height);
    }

    if (minimumY > maximumY) {
        return CustomScreenLayout{};
    }

    const float availableHeight = std::max(1.0f, viewportHeight - viewportPadding * 2.0f);
    const float contentHeight = std::max(1.0f, maximumY - minimumY);
    const float scale = std::min(1.0f, availableHeight / contentHeight);
    const float scaledMinY = minimumY * scale;
    const float scaledMaxY = maximumY * scale;
    float verticalOffset =
        alwaysAnchorToBottom ? viewportHeight - viewportPadding - scaledMaxY : preferredVerticalOffset;
    if (scaledMaxY + verticalOffset > viewportHeight - viewportPadding) {
        verticalOffset = viewportHeight - viewportPadding - scaledMaxY;
    }
    if (scaledMinY + verticalOffset < viewportPadding) {
        verticalOffset += viewportPadding - (scaledMinY + verticalOffset);
    }
    return CustomScreenLayout{scale, verticalOffset};
}

void UIRenderer::DrawCustomUI()
{
    if (!mUILoadSystem) {
        return;
    }

    const bool isUGCEditing = mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing();
    const bool isUGCPlaytestActive = mGame->GetIsUGCPlaytestActive();
    static const std::string returnToEditorText = "作る画面に戻る";
    static const std::string airDodgeAttackText = "空中回避";
    static const std::string disembarkBoatText = "降車";
    static const std::string assistAttackText = "攻撃";
    static const std::string assistRecoverText = "体力回復";
    static const std::string assistChargingText = "溜め中";
    static const std::string assistFireChargedAttackText = "発射";
    static const std::string chargedAttackReleaseGuide = "溜め完了後、離すと発動";

    const auto& elements = mUILoadSystem->GetCustomElements();
    std::vector<const UILoadSystem::CustomElement*> sortedElements;
    sortedElements.reserve(elements.size());

    for (const UILoadSystem::CustomElement& element : elements) {
        sortedElements.push_back(&element);
    }

    // 同じzOrderの要素は定義順を保ち、重なり方を変えない。
    std::stable_sort(sortedElements.begin(), sortedElements.end(),
                     [](const UILoadSystem::CustomElement* left, const UILoadSystem::CustomElement* right) {
                         return left->zOrder < right->zOrder;
                     });

    const bool shouldPreviewAll = mCustomUIPreviewEnabled && mGame->GetIsDebugEditorShowing();
    const SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool isPlaying = sceneSystem->IsPlaying();
    const bool isTitle = sceneSystem->IsTitle();
    const bool isOpening = sceneSystem->IsOpening();
    const bool isBattleStyleSelection = sceneSystem->IsBattleStyleSelection();
    const bool isTalkOrTutorial = sceneSystem->IsTalkWithNPC() || sceneSystem->IsShowingTutorialConversation();
    const std::vector<Player*>& players = mGame->GetPlayers();
    const bool isTwoPlayer = mGame->GetIsPlayer2Joined() && players.size() >= 2;
    const Player* operationPlayer =
        mGame->GetIsPlayerSplit() ? mGame->GetControlledPlayer() : (!players.empty() ? players.front() : nullptr);
    bool hasPlayerWaitingForBoat = false;
    for (const Player* player : players) {
        if (!player || !player->GetIsActive()) {
            continue;
        }
        hasPlayerWaitingForBoat = hasPlayerWaitingForBoat || player->IsWaitingForBoat();
    }
    const OperationGuideDisplayState operationGuideDisplayState{
        mGame->GetCurrentStageNum(), operationPlayer ? operationPlayer->GetCurrentPlanetNum() : 0,
        isUGCPlaytestActive || mGame->GetIsUGCMode(), isTwoPlayer,
        mGame->GetIsPauseMenuOpen() && !mGame->GetIsUGCMode()};

    std::optional<bool> canTogglePlayerSplit;

    const auto isTalkPromptElement = [](const std::string& id) {
        return id == "talkableText" || id == "talkableTextureForGameController" || id == "talkableTextureForKeyboard";
    };

    const float operationGuideViewportHeight =
        isTwoPlayer ? static_cast<float>(mFbHeight) * 0.5f : static_cast<float>(mFbHeight);
    const CustomScreenLayout operationGuideLayout =
        CalculateCustomScreenLayout(sortedElements, "operation", operationGuideViewportHeight, true, 0.0f);
    constexpr float authoredAspectRatio = 16.0f / 9.0f;
    const float authoredViewportHeight = static_cast<float>(mFbWidth) / authoredAspectRatio;
    const float centeredAuthoredViewportTop =
        std::max(0.0f, (static_cast<float>(mFbHeight) - authoredViewportHeight) * 0.5f);
    std::unordered_map<std::string, CustomScreenLayout> screenLayouts;
    for (const UILoadSystem::CustomElement* element : sortedElements) {
        if (!element || element->screen == "operation" || element->screen == "ugc" ||
            screenLayouts.contains(element->screen)) {
            continue;
        }
        screenLayouts.emplace(
            element->screen,
            CalculateCustomScreenLayout(sortedElements, element->screen, static_cast<float>(mFbHeight), false,
                                        element->screen == "title" ? centeredAuthoredViewportTop : 0.0f));
    }

    for (const UILoadSystem::CustomElement* element : sortedElements) {
        if (!element) {
            continue;
        }

        if (isUGCEditing &&
            (element->screen != "ugc" || IsBuiltInUGCEditorElement(element->id) || element->zOrder > 0)) {
            continue;
        }
        if (isUGCPlaytestActive && element->screen != "operation") {
            continue;
        }

        bool visibleInGame = mUILoadSystem->IsCustomElementVisible(*element);
        if (element->screen == "operation") {
            // ポーズ中は専用の操作表を表示するため、常設ガイドを重ねない。
            if (mGame->GetIsPauseMenuOpen() && !mGame->GetIsUGCMode() && !isUGCPlaytestActive && !shouldPreviewAll) {
                continue;
            }
            const bool isBoatDisembarkGuide =
                hasPlayerWaitingForBoat && (element->id == "buttonB" || element->id == "buttonTextB");
            visibleInGame =
                (isPlaying || operationGuideDisplayState.isPauseMenuOpen) &&
                (isBoatDisembarkGuide || ShouldShowOperationGuideElement(element->id, operationGuideDisplayState));
        } else if (element->screen == "title") {
            visibleInGame = isTitle;
        } else if (element->screen == "opening") {
            visibleInGame = isOpening;
        } else if (element->screen == "battleStyleSelection") {
            visibleInGame = isBattleStyleSelection;
            const bool isAssistStyle = sceneSystem->GetSelectedBattleStyle() == PlayerControlStyle::Assist;
            if (element->id == "easySelection") {
                visibleInGame = visibleInGame && isAssistStyle;
            } else if (element->id == "normalSelection") {
                visibleInGame = visibleInGame && !isAssistStyle;
            } else if (element->id == "easyDescription") {
                visibleInGame = visibleInGame && isAssistStyle;
            } else if (element->id == "normalDescription") {
                visibleInGame = visibleInGame && !isAssistStyle;
            }
        } else if (element->screen == "talk") {
            visibleInGame = isTalkOrTutorial;
        } else if (element->screen == "ugc") {
            // UGC用UIはステージ作成モードのゲーム画面に限定し、通常プレイでは表示しない。
            // エディター上ではDrawUGCForegroundCustomUIによる前景描画に任せる。
            visibleInGame = mGame->GetIsUGCMode() && !mGame->GetIsDebugEditorShowing();
        }
        if (isTalkOrTutorial && element->screen == "default" && isTalkPromptElement(element->id)) {
            visibleInGame = false;
        }
        if (isTwoPlayer && !isTalkOrTutorial && element->screen == "default" && isTalkPromptElement(element->id)) {
            for (std::size_t index = 0; index < 2; ++index) {
                const Player* player = players[index];
                if (!player || !sceneSystem->CanStartTalkWithNPC(player)) {
                    continue;
                }

                const bool usesController = UsesControllerUI(player);
                const bool isCorrectPromptTexture =
                    element->id == "talkableText" ||
                    (usesController && element->id == "talkableTextureForGameController") ||
                    (!usesController && element->id == "talkableTextureForKeyboard");
                if (isCorrectPromptTexture) {
                    DrawCustomElement(*element, static_cast<float>(mFbHeight) * (index == 0 ? 0.0f : 0.5f), 0.5f, true,
                                      1.0f);
                }
            }
            continue;
        }

        if (!shouldPreviewAll && !visibleInGame) {
            continue;
        }

        const std::string* textOverride =
            isUGCPlaytestActive && element->id == "buttonTextB_copy" ? &returnToEditorText : nullptr;
        if (element->id == "buttonTextY" && mGame->IsAssistControlStyle()) {
            textOverride = &assistAttackText;
        }
        if (element->id == "buttonTextX" && mGame->IsAssistControlStyle()) {
            textOverride = &assistRecoverText;
        }

        const auto resolvePlayerTextOverride = [&](const Player* player) -> const std::string* {
            if (element->id == "buttonTextA" && player && !player->GetOnGround()) {
                return &airDodgeAttackText;
            }
            if (element->id == "buttonTextB" && player && player->IsWaitingForBoat()) {
                return &disembarkBoatText;
            }
            if (element->id == "strongAttackHoldGuide" && player &&
                (player->IsSpecialCharging() || player->GetCanSpecialAttack())) {
                return &chargedAttackReleaseGuide;
            }
            if (element->id != "buttonTextX" || !player) {
                return textOverride;
            }
            if (player->GetCanSpecialAttack()) {
                return &assistFireChargedAttackText;
            }
            if (player->IsSpecialCharging()) {
                return &assistChargingText;
            }
            return mGame->IsAssistControlStyle() ? &assistRecoverText : textOverride;
        };

        if (isTwoPlayer && element->screen == "operation") {
            const std::string* player1TextOverride = resolvePlayerTextOverride(players[0]);
            DrawCustomElement(*element, operationGuideLayout.verticalOffset, operationGuideLayout.scale, false,
                              operationGuideLayout.scale, players[0],
                              CalculateOperationGuideOpacity(*element, players[0], canTogglePlayerSplit),
                              player1TextOverride);
            const std::string* player2TextOverride = resolvePlayerTextOverride(players[1]);
            DrawCustomElement(*element, static_cast<float>(mFbHeight) * 0.5f + operationGuideLayout.verticalOffset,
                              operationGuideLayout.scale, false, operationGuideLayout.scale, players[1],
                              CalculateOperationGuideOpacity(*element, players[1], canTogglePlayerSplit),
                              player2TextOverride);
            continue;
        }
        const std::string* operationTextOverride = resolvePlayerTextOverride(operationPlayer);
        const bool isOperationElement = element->screen == "operation";
        const auto screenLayout = screenLayouts.find(element->screen);
        const CustomScreenLayout elementLayout =
            isOperationElement ? operationGuideLayout
                               : (screenLayout != screenLayouts.end() ? screenLayout->second : CustomScreenLayout{});
        DrawCustomElement(*element, elementLayout.verticalOffset, elementLayout.scale, false, elementLayout.scale,
                          isOperationElement ? operationPlayer : nullptr,
                          CalculateOperationGuideOpacity(*element, operationPlayer, canTogglePlayerSplit),
                          operationTextOverride);
        CustomElementScreenTransform screenTransform;
        if (CalculateCustomElementScreenTransform(*element, screenTransform)) {
            if (elementLayout.scale != 1.0f || elementLayout.verticalOffset != 0.0f) {
                screenTransform.size *= elementLayout.scale;
                screenTransform.center.y =
                    elementLayout.verticalOffset + screenTransform.center.y * elementLayout.scale;
                if (!element->centerBased) {
                    screenTransform.center.x = mFbWidth * element->xRatio + screenTransform.size.x * 0.5f;
                }
            }
            RecordRenderedUIElement(RenderedUIElementSource::Custom, element->screen, element->id,
                                    screenTransform.center, screenTransform.size, element->rotationDegrees);
        }
    }
}

void UIRenderer::DrawUGCForegroundCustomUI(const ImVec2& viewportMin, const ImVec2& viewportSize)
{
    if (!mUILoadSystem) {
        return;
    }

    std::vector<const UILoadSystem::CustomElement*> elements;
    for (const UILoadSystem::CustomElement& element : mUILoadSystem->GetCustomElements()) {
        if (element.screen == "ugc" && element.zOrder > 0 && !IsBuiltInUGCEditorElement(element.id) &&
            element.id != "previewView" && mUILoadSystem->IsCustomElementVisible(element)) {
            elements.push_back(&element);
        }
    }
    std::stable_sort(elements.begin(), elements.end(),
                     [](const UILoadSystem::CustomElement* left, const UILoadSystem::CustomElement* right) {
                         return left->zOrder < right->zOrder;
                     });

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (const UILoadSystem::CustomElement* element : elements) {
        const float width = viewportSize.x * element->widthRatio;
        const float height = viewportSize.x * element->heightRatio;
        float x = viewportMin.x + viewportSize.x * element->xRatio;
        float y = viewportMin.y + viewportSize.x * element->yRatio;
        if (element->id == "menuIcon2_copy_copy2") {
            constexpr float authoredGroupBottomRatio = 0.5755f;
            constexpr float playHotkeyBottomMargin = 8.0f;
            const float authoredBottomRatio = element->yRatio + element->heightRatio;
            const float adjustedBottomMargin = std::max(
                0.0f, playHotkeyBottomMargin + viewportSize.x * (authoredGroupBottomRatio - authoredBottomRatio));
            y = viewportMin.y + viewportSize.y - height - adjustedBottomMargin;
        }
        if (element->centerBased) {
            x -= width * 0.5f;
            y -= height * 0.5f;
        }
        const ImVec2 min(x, y);
        const ImVec2 max(x + width, y + height);
        const ImU32 color =
            IM_COL32(static_cast<int>(element->color[0] * 255.0f), static_cast<int>(element->color[1] * 255.0f),
                     static_cast<int>(element->color[2] * 255.0f), static_cast<int>(element->color[3] * 255.0f));

        if (element->type == UILoadSystem::CustomElementType::Panel) {
            drawList->AddRectFilled(min, max, color);
        } else if (element->type == UILoadSystem::CustomElementType::Image) {
            const std::string& texturePath = ResolveCustomElementTexturePath(*element);
            if (RegisterCustomUITexture(texturePath)) {
                const GLuint texture = GetCustomUITextureHandle(texturePath);
                const bool flipVertical = ResolveCustomElementTextureFlipVertical(*element);
                drawList->AddImage(static_cast<ImTextureID>(texture), min, max,
                                   ImVec2(0.0f, flipVertical ? 1.0f : 0.0f), ImVec2(1.0f, flipVertical ? 0.0f : 1.0f),
                                   color);
            }
        } else {
            drawList->AddText(ImVec2(x, y), color, ResolveCustomElementText(*element).c_str());
        }
    }
}
