#include "gfx/UIRenderer.h"

#include "Game.h"
#include "actor/Player.h"
#include "gfx/UIShader.h"
#include "gfx/VertexArray.h"
#include "gfx/ui/HudRenderer.h"
#include "gfx/ui/OperationGuideVisibility.h"
#include "gfx/ui/PauseMenuRenderer.h"
#include "gfx/ui/SceneUIRenderer.h"
#include "gfx/ui/StateUIRenderer.h"
#include "gfx/ui/UIDebugEditorBridge.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "system/SceneSystem.h"
#include "system/text/JapaneseRubyGenerator.h"
#include "system/CameraSystem.h"
#include "system/sequence/SequenceSystem.h"
#include <algorithm>
#include <array>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
#include <optional>

void UIRenderer::DrawCustomElement(
    const UILoadSystem::CustomElement& element,
    float viewportTopY,
    float viewportScale,
    bool centerTalkPrompt,
    float contentScale,
    const Player* inputPlayer,
    float opacity,
    const std::string* textOverride)
{
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    if (contentScale < 0.0f) {
        contentScale = viewportScale;
    }
    float x = mFbWidth * element.xRatio;
    float y = viewportTopY +
              mFbWidth * element.yRatio * viewportScale;
    const float width = mFbWidth * element.widthRatio * contentScale;
    const float height = mFbWidth * element.heightRatio * contentScale;

    if (centerTalkPrompt) {
        float centerX = static_cast<float>(mFbWidth) * 0.5f;
        float centerY = viewportTopY +
                        static_cast<float>(mFbHeight) * 0.25f;
        float iconCenterX = centerX;
        float iconCenterY = centerY;
        float textCenterX = centerX;
        float textCenterY = centerY;
        for (const UILoadSystem::CustomElement& candidate :
             mUILoadSystem->GetCustomElements()) {
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
    case UILoadSystem::CustomElementType::Text:
    {
        std::string resolvedText = textOverride
            ? *textOverride
            : ResolveCustomElementText(element);
        if (inputPlayer && element.usesInputDeviceVariants) {
            const bool usesController = UsesControllerUI(inputPlayer);
            if (mGame->IsInputModifierHeld()) {
                const std::string& modifierText =
                    usesController
                        ? element.gameControllerModifierText
                        : element.keyboardModifierText;
                if (!modifierText.empty()) {
                    resolvedText = modifierText;
                }
            } else {
                const std::string& deviceText =
                    usesController
                        ? element.gameControllerText
                        : element.keyboardText;
                if (!deviceText.empty()) {
                    resolvedText = deviceText;
                }
            }
        }
        glm::vec4 textColor(
            element.color[0] * 255.0f,
            element.color[1] * 255.0f,
            element.color[2] * 255.0f,
            element.color[3] * 255.0f * opacity);
        if (element.screen == "title") {
            constexpr std::array<const char*, 3> titleMenuIds = {
                "startGame", "createStage", "playCreatedStage"};
            for (int index = 0;
                 index < static_cast<int>(titleMenuIds.size()); ++index) {
                if (element.id == titleMenuIds[index] &&
                    mGame->GetTitleMenuSelection() == index) {
                    textColor = glm::vec4(
                        255.0f, 225.0f, 45.0f, 255.0f * opacity);
                    break;
                }
            }
        }
        TextEffect effect;
        effect.shadowEnabled = element.shadowEnabled;
        effect.shadowOffset =
            static_cast<float>(mFbWidth) * contentScale *
            glm::vec2(element.shadowOffsetXRatio, element.shadowOffsetYRatio);
        effect.shadowColor = glm::vec4(
            element.shadowColor[0] * 255.0f,
            element.shadowColor[1] * 255.0f,
            element.shadowColor[2] * 255.0f,
            element.shadowColor[3] * 255.0f * opacity);
        effect.outlineEnabled = element.outlineEnabled;
        effect.outlineWidth =
            mFbWidth * element.outlineWidthRatio * contentScale;
        effect.outlineColor = glm::vec4(
            element.outlineColor[0] * 255.0f,
            element.outlineColor[1] * 255.0f,
            element.outlineColor[2] * 255.0f,
            element.outlineColor[3] * 255.0f * opacity);
        const float textScale =
            mFbWidth * element.textScaleRatio * contentScale;
        const std::vector<RubyTextSegment>& rubySegments =
            ResolveCustomElementRuby(resolvedText);
        const bool hasRuby = std::any_of(
            rubySegments.begin(),
            rubySegments.end(),
            [](const RubyTextSegment& segment) {
                return segment.showsRuby && !segment.reading.empty();
            });
        if (hasRuby) {
            constexpr float customRubyScaleRatio = 0.42f;
            constexpr float customRubyGapRatio = -0.12f;
            if (effect.shadowEnabled && effect.shadowColor.a > 0.0f) {
                DrawRubyText(
                    x + effect.shadowOffset.x,
                    y + effect.shadowOffset.y,
                    textScale,
                    customRubyScaleRatio,
                    customRubyGapRatio,
                    rubySegments,
                    effect.shadowColor,
                    element.centerBased,
                    element.rotationDegrees);
            }
            DrawRubyText(
                x,
                y,
                textScale,
                customRubyScaleRatio,
                customRubyGapRatio,
                rubySegments,
                textColor,
                element.centerBased,
                element.rotationDegrees,
                effect.outlineEnabled ? effect.outlineWidth : 0.0f,
                effect.outlineColor);
        } else {
            DrawText(
                x,
                y,
                textScale,
                resolvedText,
                element.centerBased,
                textColor,
                element.rotationDegrees,
                &effect);
        }
        break;
    }
    case UILoadSystem::CustomElementType::Image:
    {
        std::string texturePath = ResolveCustomElementTexturePath(element);
        bool flipVertical = ResolveCustomElementTextureFlipVertical(element);
        if (inputPlayer && element.usesInputDeviceVariants) {
            const bool usesController = UsesControllerUI(inputPlayer);
            const std::string& deviceTexturePath =
                usesController
                    ? element.gameControllerTexturePath
                    : element.keyboardTexturePath;
            if (!deviceTexturePath.empty()) {
                texturePath = deviceTexturePath;
                flipVertical = usesController
                    ? element.gameControllerFlipVertical
                    : element.keyboardFlipVertical;
            }
        }
        if (RegisterCustomUITexture(texturePath)) {
            const auto textureIt =
                mTextures.find(GetCustomTextureName(texturePath));
            if (textureIt != mTextures.end()) {
                DrawTextureHandle(
                    topLeftX,
                    topLeftY,
                    width,
                    height,
                    textureIt->second,
                    flipVertical,
                    element.rotationDegrees,
                    opacity);
            }
        }
        break;
    }
    case UILoadSystem::CustomElementType::Panel:
        DrawBG(
            topLeftX,
            topLeftY,
            width,
            height,
            {element.color[0], element.color[1], element.color[2],
             element.color[3] * opacity},
            element.rotationDegrees);
        break;
    }
}

void UIRenderer::DrawCustomUI()
{
    if (!mUILoadSystem) {
        return;
    }

    const bool isUGCEditing =
        mGame->GetIsUGCMode() && mGame->GetIsDebugEditorShowing();
    const bool isUGCPlaytestActive = mGame->GetIsUGCPlaytestActive();
    static const std::string returnToEditorText = "作る画面に戻る";
    static const std::string disembarkBoatText =
        "降車";
    static const std::string assistAttackText = "攻撃";
    static const std::string assistRecoverText = "体力回復";
    static const std::string assistChargingText = "溜め中";
    static const std::string assistFireChargedAttackText = "発射";
    const auto isBuiltInUGCEditorElement = [](const std::string& id) {
        return id == "presetTools" || id == "selection" || id == "menu" ||
               id == "quickTools" || id == "keyboardTools" ||
               id == "play" || id == "preview" ||
               id == "presetPlatform" || id == "presetEnemy" ||
               id == "presetPlanet" || id == "presetSwitch" ||
               id == "presetGoal" || id == "eraser" || id == "undo" ||
               id == "redo" || id == "layerUp" || id == "layerDown" ||
               id == "zoomIn" || id == "zoomOut";
    };

    const auto& elements = mUILoadSystem->GetCustomElements();
    std::vector<const UILoadSystem::CustomElement*> sortedElements;
    sortedElements.reserve(elements.size());

    for (const UILoadSystem::CustomElement& element : elements) {
        sortedElements.push_back(&element);
    }

    std::stable_sort(
        sortedElements.begin(),
        sortedElements.end(),
        [](const UILoadSystem::CustomElement* lhs, const UILoadSystem::CustomElement* rhs) {
            return lhs->zOrder < rhs->zOrder;
        });

    const bool previewAll = mCustomUIPreviewEnabled && mGame->GetIsDebugEditorShowing();
    const SceneSystem* sceneSystem = mGame->GetSceneSystem();
    const bool isPlaying = sceneSystem->IsPlaying();
    const bool isTitle = sceneSystem->IsTitle();
    const bool isOpening = sceneSystem->IsOpening();
    const bool isBattleStyleSelection =
        sceneSystem->IsBattleStyleSelection();
    const bool isTalkOrTutorial =
        sceneSystem->IsTalkWithNPC() ||
        sceneSystem->IsShowingTutorialConversation();
    const std::vector<Player*>& players = mGame->GetPlayers();
    const bool isTwoPlayer =
        mGame->GetIsPlayer2Joined() && players.size() >= 2;
    const Player* operationPlayer =
        mGame->GetIsPlayerSplit()
            ? mGame->GetControlledPlayer()
            : (!players.empty() ? players.front() : nullptr);
    bool hasPlayerWaitingForBoat = false;
    for (const Player* player : players) {
        if (!player || !player->GetIsActive()) {
            continue;
        }
        hasPlayerWaitingForBoat =
            hasPlayerWaitingForBoat || player->IsWaitingForBoat();
    }
    const OperationGuideDisplayState operationGuideDisplayState{
        mGame->GetCurrentStageNum(),
        operationPlayer ? operationPlayer->GetCurrentPlanetNum() : 0,
        isUGCPlaytestActive};

    std::optional<bool> canTogglePlayerSplit;
    const auto getOperationGuideOpacity =
        [&](const UILoadSystem::CustomElement& element,
            const Player* player) {
            constexpr float disabledOpacity = 0.38f;
            if (element.screen != "operation") {
                return 1.0f;
            }

            const bool isBoatDisembarkGuide =
                player && player->IsWaitingForBoat() &&
                (element.id == "buttonB" ||
                 element.id == "buttonTextB");
            if (isBoatDisembarkGuide) {
                return 1.0f;
            }

            const bool playerCanAct =
                player && player->GetIsActive() && player->IsAlive();
            if (!playerCanAct) {
                return disabledOpacity;
            }

            const bool isSpecialCharging =
                player->IsSpecialCharging() || player->GetCanSpecialAttack();
            const bool isContinuousAttack =
                player->IsContinuousAttacking();
            // 連続攻撃は地上にいる間だけ操作を占有する。空中では残り時間が
            // 減るだけで、弱攻撃・回避・強攻撃を通常どおり受け付ける。
            const bool isGroundContinuousAttack =
                isContinuousAttack && player->GetOnGround();
            const bool isModifierHeld = mGame->IsInputModifierHeld();
            const bool isIdle =
                player->GetActionState() == Player::ActionState::Idle;
            const bool isWeakAttacking =
                player->GetActionState() == Player::ActionState::Attacking;
            const bool canStartNormalAction =
                isIdle && !isSpecialCharging && !isGroundContinuousAttack;
            const bool canRecover =
                isIdle && !isSpecialCharging &&
                !player->GetCanSpecialAttack() &&
                player->GetJewelCount() >= 1 &&
                player->GetHp() < player->GetMaxHp();
            const bool canStartChargeAttack =
                isIdle && !isGroundContinuousAttack &&
                player->GetJewelCount() >= 2;
            const bool canStartContinuousAttack =
                canStartNormalAction && player->GetOnGround() &&
                player->GetJewelCount() >=
                    PlayerJewelGauge::ContinuousAttackCost;
            bool isEnabled = true;

            if (element.id == "buttonA" || element.id == "buttonTextA") {
                if (isModifierHeld) {
                    // 特殊入力中は「体力回復」。ジュエルを1個消費する。
                    isEnabled = canRecover;
                    return isEnabled ? 1.0f : disabledOpacity;
                }



                isEnabled = isIdle && !isSpecialCharging &&
                            player->GetOnGround();
            } else if (element.id == "buttonB" ||
                       element.id == "buttonTextB") {
                // 回避のクールタイムは短く、UIまで点滅すると見づらい。
                // 継続攻撃・溜めなど、操作が明確に封じられる状態だけ示す。
                // 溜め中の回避は実際に受け付けるため、通常表示を保つ。



                isEnabled = !isGroundContinuousAttack &&
                            (isIdle || isWeakAttacking);
            } else if (element.id == "buttonA_copy" ||
                       element.id == "buttonTextB_copy") {
                isEnabled = mGame->CanOpenPauseMenu();
            } else if (element.id == "buttonX" ||
                       element.id == "buttonTextX") {
                if (isModifierHeld) {
                    // 特殊入力中は「溜め攻撃」。開始には2個必要で、溜め中は
                    // 同じ入力で発射／終了する。
                    isEnabled = isSpecialCharging || canStartChargeAttack;
                    return isEnabled ? 1.0f : disabledOpacity;
                }
                if (mGame->IsAssistControlStyle()) {
                    isEnabled = isSpecialCharging ||
                                player->GetCanSpecialAttack() ||
                                canRecover;
                    return isEnabled ? 1.0f : disabledOpacity;
                }
                // 溜め攻撃は同じ強攻撃入力で発射／終了するので、その間も
                // 強攻撃だけは有効として表示する。
                isEnabled = canStartNormalAction || isSpecialCharging;
            } else if (element.id == "buttonY" ||
                       element.id == "buttonTextY") {
                // 特殊入力中は「連続攻撃」で、ジュエルを2個消費する。
                isEnabled = isModifierHeld
                                ? canStartContinuousAttack
                                : canStartNormalAction;
            } else if (element.id == "buttonB_copy" ||
                       element.id == "buttonTextB_copy2") {
                // 溜め中は移動入力を受け付けない。一方、連続攻撃中は
                // 移動できるため通常表示を保つ。
                isEnabled = !isSpecialCharging && isIdle;
            } else if (element.id == "buttonB_copy_copy" ||
                       element.id == "buttonTextB_copy2_copy" ||
                       element.id == "buttonB_copy_copy_copy" ||
                       element.id == "buttonTextB_copy2_copy_copy") {


                const CameraSystem* cameraSystem = mGame->GetCameraSystem();
                isEnabled = cameraSystem && cameraSystem->AllowsPlayerInput();
            } else if (element.id == "buttonB_copy_copy2" ||
                       element.id == "buttonTextB_copy2_copy2") {
                isEnabled = mGame->CanSwitchControlledPlayer();
            } else if (element.id == "buttonB_copy_copy2_copy" ||
                       element.id == "buttonTextB_copy2_copy2_copy") {
                if (!canTogglePlayerSplit) {
                    canTogglePlayerSplit =
                        mGame->CanTogglePlayerSplit();
                }
                isEnabled = *canTogglePlayerSplit;
            }

            return isEnabled ? 1.0f : disabledOpacity;
        };

    const auto isTalkPromptElement = [](const std::string& id) {
        return id == "talkableText" ||
               id == "talkableTextureForGameController" ||
               id == "talkableTextureForKeyboard";
    };





    const auto getOperationGuideVerticalOffset = [&]() {
        if (!isTwoPlayer) {
            return 0.0f;
        }




        constexpr float positionScale = 1.0f;
        constexpr float viewportPadding = 12.0f;
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        for (const UILoadSystem::CustomElement* candidate : sortedElements) {
            if (!candidate || candidate->screen != "operation") {
                continue;
            }

            float height = mFbWidth * candidate->heightRatio;
            if (candidate->type == UILoadSystem::CustomElementType::Text) {
                const std::string resolvedText =
                    ResolveCustomElementText(*candidate);
                std::string firstLine;
                std::string secondLine;
                const bool hasSecondLine =
                    SplitText(resolvedText, firstLine, secondLine);
                int firstWidth = 0;
                int firstHeight = 0;
                MeasureText(
                    firstLine,
                    mFbWidth * candidate->textScaleRatio,
                    firstWidth,
                    firstHeight);
                height = static_cast<float>(firstHeight);
                if (hasSecondLine) {
                    int secondWidth = 0;
                    int secondHeight = 0;
                    MeasureText(
                        secondLine,
                        mFbWidth * candidate->textScaleRatio,
                        secondWidth,
                        secondHeight);
                    height += static_cast<float>(secondHeight) +
                              mFbHeight * 0.0666f;
                }
            }

            const float y = mFbWidth * candidate->yRatio * positionScale;
            const float top = candidate->centerBased ? y - height * 0.5f : y;
            minY = std::min(minY, top);
            maxY = std::max(maxY, top + height);
        }

        if (minY > maxY) {
            return 0.0f;
        }

        const float viewportHeight = mFbHeight * 0.5f;
        float offset = 0.0f;
        if (maxY > viewportHeight - viewportPadding) {
            offset = viewportHeight - viewportPadding - maxY;
        }
        if (minY + offset < viewportPadding) {
            offset += viewportPadding - (minY + offset);
        }
        return offset;
    };
    const float operationGuideVerticalOffset =
        getOperationGuideVerticalOffset();

    for (const UILoadSystem::CustomElement* element : sortedElements) {
        if (!element) {
            continue;
        }

        if (isUGCEditing &&
            (element->screen != "ugc" ||
             isBuiltInUGCEditorElement(element->id) ||
             element->zOrder > 0)) {
            continue;
        }
        if (isUGCPlaytestActive && element->screen != "operation") {
            continue;
        }

        bool visibleInGame = mUILoadSystem->IsCustomElementVisible(*element);
        if (element->screen == "operation") {
            const bool isBoatDisembarkGuide =
                hasPlayerWaitingForBoat &&
                (element->id == "buttonB" ||
                 element->id == "buttonTextB");
            visibleInGame =
                isPlaying &&
                (isBoatDisembarkGuide ||
                 ShouldShowOperationGuideElement(
                     element->id,
                     operationGuideDisplayState));
        } else if (element->screen == "title") {
            visibleInGame = isTitle;
        } else if (element->screen == "opening") {
            visibleInGame = isOpening;
        } else if (element->screen == "battleStyleSelection") {
            visibleInGame = isBattleStyleSelection;
            const bool isAssistStyle =
                sceneSystem->GetSelectedBattleStyle() ==
                PlayerControlStyle::Assist;
            if (element->id == "easySelection") {
                visibleInGame =
                    visibleInGame &&
                    isAssistStyle;
            } else if (element->id == "normalSelection") {
                visibleInGame =
                    visibleInGame &&
                    !isAssistStyle;
            } else if (element->id == "easyDescription") {


                visibleInGame = visibleInGame && isAssistStyle;
            } else if (element->id == "normalDescription") {
                visibleInGame = visibleInGame && !isAssistStyle;
            }
        } else if (element->screen == "talk") {


            visibleInGame = isTalkOrTutorial;
        } else if (element->screen == "ugc") {
            // UGC用のカスタムUIは、ステージ作成モードのゲーム画面だけに
            // 限定する。通常プレイでは visibleByDefault に関係なく出さない。
            // デバッグエディター上では下の専用前景描画に任せる。
            visibleInGame = mGame->GetIsUGCMode() &&
                            !mGame->GetIsDebugEditorShowing();
        }
        if (isTalkOrTutorial && element->screen == "default" &&
            isTalkPromptElement(element->id)) {


            visibleInGame = false;
        }
        if (isTwoPlayer && !isTalkOrTutorial &&
            element->screen == "default" &&
            isTalkPromptElement(element->id)) {


            for (std::size_t index = 0; index < 2; ++index) {
                const Player* player = players[index];
                if (!player || !sceneSystem->CanStartTalkWithNPC(player)) {
                    continue;
                }

                const bool usesController = UsesControllerUI(player);
                const bool isCorrectPromptTexture =
                    element->id == "talkableText" ||
                    (usesController &&
                     element->id == "talkableTextureForGameController") ||
                    (!usesController &&
                     element->id == "talkableTextureForKeyboard");
                if (isCorrectPromptTexture) {
                    DrawCustomElement(
                        *element,
                        static_cast<float>(mFbHeight) *
                            (index == 0 ? 0.0f : 0.5f),
                        0.5f,
                        true,
                        1.0f);
                }
            }
            continue;
        }

        if (!previewAll && !visibleInGame) {
            continue;
        }

        const std::string* textOverride =
            isUGCPlaytestActive && element->id == "buttonTextB_copy"
                ? &returnToEditorText
                : nullptr;
        if (element->id == "buttonTextY" &&
            mGame->IsAssistControlStyle() &&
            !mGame->IsInputModifierHeld()) {
            textOverride = &assistAttackText;
        }
        if (element->id == "buttonTextX" &&
            mGame->IsAssistControlStyle() &&
            !mGame->IsInputModifierHeld()) {
            textOverride = &assistRecoverText;
        }

        const auto resolvePlayerTextOverride =
            [&](const Player* player) -> const std::string* {
                if (element->id == "buttonTextB" && player &&
                    player->IsWaitingForBoat()) {
                    return &disembarkBoatText;
                }
                if (element->id != "buttonTextX" ||
                    !mGame->IsAssistControlStyle() ||
                    !player) {
                    return textOverride;
                }
                if (player->GetCanSpecialAttack()) {
                    return &assistFireChargedAttackText;
                }
                if (player->IsSpecialCharging()) {
                    return &assistChargingText;
                }
                if (mGame->IsInputModifierHeld()) {
                    return textOverride;
                }
                return &assistRecoverText;
            };

        if (isTwoPlayer && element->screen == "operation") {
            const std::string* player1TextOverride =
                resolvePlayerTextOverride(players[0]);
            DrawCustomElement(
                *element,
                operationGuideVerticalOffset,
                1.0f,
                false,
                1.0f,
                players[0],
                getOperationGuideOpacity(*element, players[0]),
                player1TextOverride);
            const std::string* player2TextOverride =
                resolvePlayerTextOverride(players[1]);
            DrawCustomElement(
                *element,
                static_cast<float>(mFbHeight) * 0.5f +
                    operationGuideVerticalOffset,
                1.0f,
                false,
                1.0f,
                players[1],
                getOperationGuideOpacity(*element, players[1]),
                player2TextOverride);
            continue;
        }
        const std::string* operationTextOverride =
            resolvePlayerTextOverride(operationPlayer);
        DrawCustomElement(
            *element,
            0.0f,
            1.0f,
            false,
            -1.0f,
            element->screen == "operation" ? operationPlayer : nullptr,
            getOperationGuideOpacity(*element, operationPlayer),
            operationTextOverride);
        CustomElementScreenTransform screenTransform;
        if (CalculateCustomElementScreenTransform(*element, screenTransform)) {
            RecordRenderedUIElement(
                RenderedUIElementSource::Custom,
                element->screen,
                element->id,
                screenTransform.center,
                screenTransform.size,
                element->rotationDegrees);
        }
    }
}

void UIRenderer::DrawUGCForegroundCustomUI(
    const ImVec2& viewportMin,
    const ImVec2& viewportSize)
{
    if (!mUILoadSystem) {
        return;
    }

    const auto isBuiltInUGCEditorElement = [](const std::string& id) {
        return id == "presetTools" || id == "selection" || id == "menu" ||
               id == "quickTools" || id == "keyboardTools" ||
               id == "play" || id == "preview" ||
               id == "presetPlatform" || id == "presetEnemy" ||
               id == "presetPlanet" || id == "presetSwitch" ||
               id == "presetGoal" || id == "eraser" || id == "undo" ||
               id == "redo" || id == "layerUp" || id == "layerDown" ||
               id == "zoomIn" || id == "zoomOut" || id == "previewView";
    };

    std::vector<const UILoadSystem::CustomElement*> elements;
    for (const UILoadSystem::CustomElement& element :
         mUILoadSystem->GetCustomElements()) {
        if (element.screen == "ugc" && element.zOrder > 0 &&
            !isBuiltInUGCEditorElement(element.id) &&
            mUILoadSystem->IsCustomElementVisible(element)) {
            elements.push_back(&element);
        }
    }
    std::stable_sort(
        elements.begin(), elements.end(),
        [](const UILoadSystem::CustomElement* left,
           const UILoadSystem::CustomElement* right) {
            return left->zOrder < right->zOrder;
        });

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    for (const UILoadSystem::CustomElement* element : elements) {
        const float width = viewportSize.x * element->widthRatio;
        const float height = viewportSize.x * element->heightRatio;
        float x = viewportMin.x + viewportSize.x * element->xRatio;
        float y = viewportMin.y + viewportSize.x * element->yRatio;
        if (element->centerBased) {
            x -= width * 0.5f;
            y -= height * 0.5f;
        }
        const ImVec2 min(x, y);
        const ImVec2 max(x + width, y + height);
        const ImU32 color = IM_COL32(
            static_cast<int>(element->color[0] * 255.0f),
            static_cast<int>(element->color[1] * 255.0f),
            static_cast<int>(element->color[2] * 255.0f),
            static_cast<int>(element->color[3] * 255.0f));

        if (element->type == UILoadSystem::CustomElementType::Panel) {
            drawList->AddRectFilled(min, max, color);
        } else if (element->type == UILoadSystem::CustomElementType::Image) {
            const std::string& texturePath =
                ResolveCustomElementTexturePath(*element);
            if (RegisterCustomUITexture(texturePath)) {
                const GLuint texture = GetCustomUITextureHandle(texturePath);
                const bool flipVertical =
                    ResolveCustomElementTextureFlipVertical(*element);
                drawList->AddImage(
                    static_cast<ImTextureID>(texture), min, max,
                    ImVec2(0.0f, flipVertical ? 1.0f : 0.0f),
                    ImVec2(1.0f, flipVertical ? 0.0f : 1.0f), color);
            }
        } else {
            drawList->AddText(
                ImVec2(x, y), color,
                ResolveCustomElementText(*element).c_str());
        }
    }
}
