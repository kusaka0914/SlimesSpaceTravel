#include "gfx/debug/stage/StageNPCInspector.h"

#include "Game.h"
#include "Stage.h"
#include "actor/NPC.h"
#include "actor/Planet.h"
#include "actor/TutorialTrigger.h"
#include "gfx/debug/stage/StageActorAssetEditor.h"
#include "gfx/debug/stage/StageActorQuery.h"
#include "imgui.h"
#include "system/SceneSystem.h"
#include "system/scene/TutorialController.h"
#include "system/text/JapaneseRubyGenerator.h"
#include "system/tutorial/TutorialLibrary.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <utility>
#include <vector>

StageNPCInspector::StageNPCInspector(
    DebugEditorContext& context,
    StageActorAssetEditor& assetEditor)
    : mContext(context),
      mAssetEditor(assetEditor)
{
}

void StageNPCInspector::Draw(
    NPC* npc,
    const std::string& sequenceName,
    std::size_t listIndex,
    int yamlIndex)
{
    if (!npc) {
        return;
    }

        TutorialTrigger* tutorialTrigger =
            dynamic_cast<TutorialTrigger*>(npc);
        const bool isTutorialTrigger =
            tutorialTrigger != nullptr;
        ImGui::SeparatorText(
            isTutorialTrigger
                ? "チュートリアルトリガー設定"
                : "NPC・会話設定");

        if (!isTutorialTrigger) {
            mAssetEditor.DrawNPCModelPicker(
                npc,
                sequenceName,
                listIndex);

            std::array<char, 128> nameBuffer = {};
            std::snprintf(
                nameBuffer.data(),
                nameBuffer.size(),
                "%s",
                npc->GetName().c_str());
            if (ImGui::InputText(
                    ("NPC名##placedNPCName" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    nameBuffer.data(),
                    nameBuffer.size())) {
                npc->SetName(nameBuffer.data());
            }

            bool forcesTalkOnArrival =
                npc->GetForcesTalkOnArrival();
            if (ImGui::Checkbox(
                    ("到着時に未読会話を強制開始##forceTalkOnArrival" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &forcesTalkOnArrival)) {
                if (forcesTalkOnArrival) {
                    Stage* stage = mContext.game->GetCurrentStage();
                    if (stage) {
                        for (Planet* planet : stage->GetPlanets()) {
                            if (!planet) {
                                continue;
                            }
                            for (NPC* otherNPC : planet->GetNPCs()) {
                                if (otherNPC && otherNPC != npc) {
                                    otherNPC->SetForcesTalkOnArrival(false);
                                }
                            }
                        }
                    }
                }
                npc->SetForcesTalkOnArrival(forcesTalkOnArrival);
            }
            ImGui::TextDisabled(
                "拠点・惑星への到着演出後、現在のクリア状況に対応する会話が未読なら一度だけ開始します。");
            ImGui::TextDisabled(
                "有効にできるNPCはステージ内で1人だけです。到着した惑星に所属する場合だけ開始します。");

            float talkRadius = npc->GetRadius();
            if (ImGui::DragFloat(
                    ("会話判定の半径##placedNPCRadius" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    &talkRadius,
                    0.05f,
                    0.1f,
                    20.0f,
                    "%.2f")) {
                npc->SetRadius(std::max(0.1f, talkRadius));
            }
            ImGui::TextDisabled(
                "実際の会話可能距離は、この半径に0.5を加えた値です。");
        } else {
            mAssetEditor.DrawActorModelPicker(
                npc,
                sequenceName,
                listIndex);
            ImGui::TextDisabled(
                "モデルの位置・回転・スケールが、そのままトリガー範囲になります。");
            ImGui::TextDisabled(
                "箱型モデル以外では、モデル全体を囲む箱として判定します。");

            TutorialController* tutorialController =
                mContext.game && mContext.game->GetSceneSystem()
                    ? mContext.game->GetSceneSystem()
                          ->GetTutorialController()
                    : nullptr;
            TutorialLibrary* tutorialLibrary =
                tutorialController
                    ? &tutorialController->GetLibrary()
                    : nullptr;
            const std::string tutorialPreview =
                tutorialTrigger->GetTutorialId().empty()
                    ? "従来の直接入力を使用"
                    : tutorialTrigger->GetTutorialId();
            if (ImGui::BeginCombo(
                    ("再生するチュートリアル##tutorialId" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    tutorialPreview.c_str())) {
                const bool usesLegacyTalk =
                    tutorialTrigger->GetTutorialId().empty();
                if (ImGui::Selectable(
                        "従来の直接入力を使用",
                        usesLegacyTalk)) {
                    tutorialTrigger->SetTutorialId("");
                }

                if (tutorialLibrary) {
                    for (const TutorialDefinition& definition :
                         tutorialLibrary->GetDefinitions()) {
                        const bool selected =
                            tutorialTrigger->GetTutorialId() ==
                            definition.id;
                        const std::string label =
                            definition.displayName + " (" +
                            definition.id + ")##triggerTutorial" +
                            std::to_string(yamlIndex) + definition.id;
                        if (ImGui::Selectable(
                                label.c_str(), selected)) {
                            tutorialTrigger->SetTutorialId(
                                definition.id);
                        }
                    }
                }
                ImGui::EndCombo();
            }

            if (!tutorialTrigger->GetTutorialId().empty() &&
                tutorialController &&
                ImGui::Button(
                    ("このチュートリアルをプレビュー##triggerTutorialPreview" +
                     std::to_string(yamlIndex))
                        .c_str())) {
                tutorialController->Preview(
                    tutorialTrigger->GetTutorialId());
            }

            const std::string prerequisitePreview =
                tutorialTrigger->GetRequiredCompletedTutorialId().empty()
                    ? "前提なし"
                    : tutorialTrigger->GetRequiredCompletedTutorialId();
            if (ImGui::BeginCombo(
                    ("発動に必要な完了済みチュートリアル##tutorialPrerequisite" +
                     std::to_string(yamlIndex))
                        .c_str(),
                    prerequisitePreview.c_str())) {
                if (ImGui::Selectable(
                        "前提なし",
                        tutorialTrigger->GetRequiredCompletedTutorialId()
                            .empty())) {
                    tutorialTrigger->SetRequiredCompletedTutorialId("");
                }
                if (tutorialLibrary) {
                    for (const TutorialDefinition& definition :
                         tutorialLibrary->GetDefinitions()) {
                        if (definition.id == tutorialTrigger->GetTutorialId()) {
                            continue;
                        }
                        const bool selected =
                            tutorialTrigger->GetRequiredCompletedTutorialId() ==
                            definition.id;
                        const std::string label =
                            definition.displayName + " (" + definition.id +
                            ")##triggerTutorialPrerequisite" +
                            std::to_string(yamlIndex) + definition.id;
                        if (ImGui::Selectable(label.c_str(), selected)) {
                            tutorialTrigger->SetRequiredCompletedTutorialId(
                                definition.id);
                        }
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::TextDisabled(
                "内容はデバッグエディターの「チュートリアル」タブで編集します。");
        }

        const bool shouldDrawInlineConversationEditor = !isTutorialTrigger || tutorialTrigger->GetTutorialId().empty();
        if (shouldDrawInlineConversationEditor) {
            int proximityMessageMode = static_cast<int>(npc->GetProximityMessageMode());
            if (!isTutorialTrigger) {
                ImGui::SeparatorText("頭上のひとこと表示");

                constexpr const char* proximityModeLabels[] = {"使用しない", "通常会話を終えた後",
                                                               "最初から表示のみ（会話不可）"};
                proximityMessageMode = std::clamp(proximityMessageMode, 0, 2);
                if (ImGui::Combo(("表示タイミング##npcProximityMessageMode" + std::to_string(yamlIndex)).c_str(),
                                 &proximityMessageMode, proximityModeLabels, IM_ARRAYSIZE(proximityModeLabels))) {
                    npc->SetProximityMessageMode(static_cast<NPCProximityMessageMode>(proximityMessageMode));
                }

                if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                    float proximityRange = npc->GetProximityMessageRange();
                    if (ImGui::DragFloat(
                            ("表示される距離##npcProximityMessageRange" + std::to_string(yamlIndex)).c_str(),
                            &proximityRange, 0.05f, 0.1f, 30.0f, "%.2f")) {
                        npc->SetProximityMessageRange(proximityRange);
                    }

                    float proximityHeight = npc->GetProximityMessageHeight();
                    if (ImGui::DragFloat(("頭上の高さ##npcProximityMessageHeight" + std::to_string(yamlIndex)).c_str(),
                                         &proximityHeight, 0.05f, 0.0f, 20.0f, "%.2f")) {
                        npc->SetProximityMessageHeight(proximityHeight);
                    }

                    float proximityScale = npc->GetProximityMessageScale();
                    if (ImGui::DragFloat(
                            ("吹き出しの大きさ##npcProximityMessageScale" + std::to_string(yamlIndex)).c_str(),
                            &proximityScale, 0.02f, 0.1f, 5.0f, "%.2f")) {
                        npc->SetProximityMessageScale(proximityScale);
                    }

                    ImGui::TextDisabled("エディターを開いている間は、距離や会話済みに関係なくプレビュー表示します。");
                    if (proximityMessageMode == static_cast<int>(NPCProximityMessageMode::AfterTalk)) {
                        ImGui::TextDisabled(
                            "会話を最後まで読んだ後は再び話しかけられず、近づくとこの一言を表示します。");
                    } else {
                        ImGui::TextDisabled("このNPCには話しかけられず、近づくとこの一言だけを表示します。");
                    }
                    ImGui::TextDisabled("一言の内容は、下にある各通常会話の設定内で入力します。");
                }
            }

            const std::vector<std::string>& talkTexts = npc->GetTalkTexts();
            const std::vector<StageActorInstance> talkFocusCandidates =
                StageActorQuery::CollectAllActorInstances(mContext.game->GetCurrentStage());

            ImGui::TextDisabled("ルビは全会話に自動生成されます。必要な箇所だけ読みを修正できます。");

            for (std::size_t talkIndex = 0; talkIndex < talkTexts.size(); ++talkIndex) {
                std::array<char, 1024> talkTextBuffer = {};
                std::snprintf(talkTextBuffer.data(), talkTextBuffer.size(), "%s", talkTexts[talkIndex].c_str());

                const std::string talkLabel = "会話 " + std::to_string(talkIndex + 1) + "##placedNPCTalk" +
                                              std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                if (ImGui::InputTextMultiline(talkLabel.c_str(), talkTextBuffer.data(), talkTextBuffer.size(),
                                              ImVec2(-1.0f, 70.0f))) {
                    npc->SetTalkText(talkIndex, talkTextBuffer.data());
                    std::vector<RubyTextSegment> generatedSegments;
                    std::string errorMessage;
                    if (JapaneseRubyGenerator::Generate(talkTextBuffer.data(), generatedSegments, errorMessage)) {
                        npc->SetTalkRubySegments(talkIndex, std::move(generatedSegments));
                        mRubyGenerationStatus = "本文に合わせてルビを自動更新しました。";
                    } else {
                        mRubyGenerationStatus = errorMessage.empty() ? "ルビの生成に失敗しました。" : errorMessage;
                    }
                }

                const std::vector<RubyTextSegment>& rubySegments = npc->GetTalkRubySegments(talkIndex);
                if (npc->HasValidTalkRuby(talkIndex)) {
                    const std::string rubyTreeId = "ルビの読みを修正##placedNPCRubyEdit" + std::to_string(yamlIndex) +
                                                   "_" + std::to_string(talkIndex);
                    if (ImGui::TreeNode(rubyTreeId.c_str())) {
                        for (std::size_t segmentIndex = 0; segmentIndex < rubySegments.size(); ++segmentIndex) {
                            const RubyTextSegment& segment = rubySegments[segmentIndex];
                            if (!segment.showsRuby) {
                                continue;
                            }

                            ImGui::Text("「%s」", segment.text.c_str());
                            ImGui::SameLine();

                            std::array<char, 256> readingBuffer = {};
                            std::snprintf(readingBuffer.data(), readingBuffer.size(), "%s", segment.reading.c_str());
                            const std::string readingInputId = "##placedNPCRubyReading" + std::to_string(yamlIndex) +
                                                               "_" + std::to_string(talkIndex) + "_" +
                                                               std::to_string(segmentIndex);
                            if (ImGui::InputText(readingInputId.c_str(), readingBuffer.data(), readingBuffer.size())) {
                                npc->SetTalkRubyReading(talkIndex, segmentIndex, readingBuffer.data());
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                if (!mRubyGenerationStatus.empty()) {
                    ImGui::TextDisabled("%s", mRubyGenerationStatus.c_str());
                }

                int talkStageCondition = npc->GetTalkStageClearCondition(talkIndex);
                bool usesTalkStageCondition = talkStageCondition >= 0;
                if (ImGui::Checkbox(("ステージクリア後の会話##npcTalkStageConditionEnabled" +
                                     std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                        .c_str(),
                                    &usesTalkStageCondition)) {
                    if (usesTalkStageCondition) {
                        talkStageCondition = std::max(0, mContext.game ? mContext.game->GetCurrentStageNum() : 0);
                    } else {
                        talkStageCondition = -1;
                    }
                    npc->SetTalkStageClearCondition(talkIndex, talkStageCondition);
                }

                if (usesTalkStageCondition) {
                    const std::string stagePreview = "ステージ " + std::to_string(talkStageCondition);
                    const std::string conditionComboId = "クリア済み条件##npcTalkStageCondition" +
                                                         std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                    if (ImGui::BeginCombo(conditionComboId.c_str(), stagePreview.c_str())) {
                        const int stageCount = mContext.game ? static_cast<int>(mContext.game->GetStages().size()) : 0;
                        for (int stageNum = 0; stageNum < stageCount; ++stageNum) {
                            const bool selected = talkStageCondition == stageNum;
                            const std::string label = "ステージ " + std::to_string(stageNum) +
                                                      "##npcTalkStageConditionOption" + std::to_string(yamlIndex) +
                                                      "_" + std::to_string(talkIndex) + "_" + std::to_string(stageNum);
                            if (ImGui::Selectable(label.c_str(), selected)) {
                                talkStageCondition = stageNum;
                                npc->SetTalkStageClearCondition(talkIndex, stageNum);
                            }
                        }
                        ImGui::EndCombo();
                    }
                } else {
                    ImGui::TextDisabled("この会話は未クリア時の通常会話に含まれます。");
                }

                bool startsOpeningAfterPage =
                    npc->GetTalkStartsOpeningAfterPage(talkIndex);
                if (ImGui::Checkbox(
                        ("このページの後にオープニングを再生##npcTalkOpeningAfter" +
                         std::to_string(yamlIndex) + "_" +
                         std::to_string(talkIndex))
                            .c_str(),
                        &startsOpeningAfterPage)) {
                    npc->SetTalkStartsOpeningAfterPage(
                        talkIndex, startsOpeningAfterPage);
                }
                ImGui::TextDisabled(
                    "ストーリー終了後、フェードを挟んで次の会話ページへ戻ります。");

                bool startsEndingAfterPage =
                    npc->GetTalkStartsEndingAfterPage(talkIndex);
                if (ImGui::Checkbox(
                        ("全ての星を集めた後、このページの後にエンディングを再生##npcTalkEndingAfter" +
                         std::to_string(yamlIndex) + "_" +
                         std::to_string(talkIndex))
                            .c_str(),
                        &startsEndingAfterPage)) {
                    npc->SetTalkStartsEndingAfterPage(
                        talkIndex, startsEndingAfterPage);
                }
                ImGui::TextDisabled(
                    "ステージ1〜5を全てクリア済みのときだけ有効です。終了後はエンドロールへ進みます。");

                if (isTutorialTrigger) {
                    constexpr const char* advanceConditionLabels[] = {"決定ボタンで進む", "分身切替成功で進む",
                                                                      "ジャンプ後の着地で進む"};
                    int advanceCondition = static_cast<int>(npc->GetTalkAdvanceCondition(talkIndex));
                    const std::string advanceConditionId = "進行条件##tutorialAdvanceCondition" +
                                                           std::to_string(yamlIndex) + "_" + std::to_string(talkIndex);
                    if (ImGui::Combo(advanceConditionId.c_str(), &advanceCondition, advanceConditionLabels,
                                     IM_ARRAYSIZE(advanceConditionLabels))) {
                        npc->SetTalkAdvanceCondition(talkIndex,
                                                     static_cast<TalkPageAdvanceCondition>(advanceCondition));
                    }

                    if (advanceCondition == static_cast<int>(TalkPageAdvanceCondition::Confirm)) {
                        ImGui::TextDisabled("通常の会話と同じく、決定ボタンで次へ進みます。");
                    } else {
                        ImGui::TextDisabled("操作が成功するまで決定ボタンでは進みません。");
                        ImGui::TextDisabled("待機中は操作中のプレイヤーだけが動き、敵や足場ギミックは停止します。");
                    }
                }

                if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                    std::array<char, 512> proximityTextBuffer = {};
                    std::snprintf(proximityTextBuffer.data(), proximityTextBuffer.size(), "%s",
                                  npc->GetTalkProximityMessageText(talkIndex).c_str());
                    if (ImGui::InputText(("この会話に対応する頭上一言##npcTalkProximityMessageText" +
                                          std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                             .c_str(),
                                         proximityTextBuffer.data(), proximityTextBuffer.size())) {
                        npc->SetTalkProximityMessageText(talkIndex, proximityTextBuffer.data());

                        const std::string& proximityText = npc->GetTalkProximityMessageText(talkIndex);
                        if (proximityText.empty()) {
                            npc->ClearTalkProximityMessageRubySegments(talkIndex);
                        } else {
                            std::vector<RubyTextSegment> generatedProximitySegments;
                            std::string errorMessage;
                            if (JapaneseRubyGenerator::Generate(proximityText, generatedProximitySegments,
                                                                errorMessage)) {
                                npc->SetTalkProximityMessageRubySegments(talkIndex,
                                                                         std::move(generatedProximitySegments));
                                mRubyGenerationStatus = "頭上一言に合わせてルビを自動更新しました。";
                            } else {
                                mRubyGenerationStatus =
                                    errorMessage.empty() ? "頭上一言のルビ生成に失敗しました。" : errorMessage;
                            }
                        }
                    }

                    const std::vector<RubyTextSegment>& proximityRubySegments =
                        npc->GetTalkProximityMessageRubySegments(talkIndex);
                    if (npc->HasValidTalkProximityMessageRuby(talkIndex)) {
                        const std::string proximityRubyTreeId = "頭上一言のルビを修正##npcProximityRubyEdit" +
                                                                std::to_string(yamlIndex) + "_" +
                                                                std::to_string(talkIndex);
                        if (ImGui::TreeNode(proximityRubyTreeId.c_str())) {
                            for (std::size_t segmentIndex = 0; segmentIndex < proximityRubySegments.size();
                                 ++segmentIndex) {
                                const RubyTextSegment& segment = proximityRubySegments[segmentIndex];
                                if (!segment.showsRuby) {
                                    continue;
                                }

                                ImGui::Text("「%s」", segment.text.c_str());
                                ImGui::SameLine();

                                std::array<char, 256> readingBuffer = {};
                                std::snprintf(readingBuffer.data(), readingBuffer.size(), "%s",
                                              segment.reading.c_str());
                                const std::string readingInputId =
                                    "##npcProximityRubyReading" + std::to_string(yamlIndex) + "_" +
                                    std::to_string(talkIndex) + "_" + std::to_string(segmentIndex);
                                if (ImGui::InputText(readingInputId.c_str(), readingBuffer.data(),
                                                     readingBuffer.size())) {
                                    npc->SetTalkProximityMessageRubyReading(talkIndex, segmentIndex,
                                                                            readingBuffer.data());
                                }
                            }
                            ImGui::TreePop();
                        }
                    }
                    ImGui::TextDisabled("この通常会話がクリア状況によって選ばれたときに使われます。");
                }

                const NPCTalkCameraFocusTarget* currentFocus = npc->GetTalkCameraFocusTarget(talkIndex);
                const bool hasCurrentFocus = currentFocus != nullptr;
                const std::string currentFocusSequence = currentFocus ? currentFocus->sequenceName : std::string();
                const int currentFocusIndex = currentFocus ? currentFocus->yamlIndex : -1;
                std::string focusPreview = "フォーカスなし";
                bool focusTargetFound = !currentFocus;

                if (currentFocus) {
                    for (const StageActorInstance& candidate : talkFocusCandidates) {
                        if (candidate.ref.sequenceName != currentFocus->sequenceName ||
                            candidate.ref.yamlIndex != currentFocus->yamlIndex) {
                            continue;
                        }

                        focusPreview = StageActorQuery::GetTypeLabel(candidate.ref) + " / " + candidate.ref.label;
                        if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                            targetNPC && !targetNPC->GetName().empty()) {
                            focusPreview += " (" + targetNPC->GetName() + ")";
                        }
                        focusTargetFound = true;
                        break;
                    }
                }

                if (!focusTargetFound && currentFocus) {
                    focusPreview = "対象が見つかりません (" + currentFocus->sequenceName + ":" +
                                   std::to_string(currentFocus->yamlIndex) + ")";
                }

                const std::string focusComboId = "カメラフォーカス##placedNPCTalkFocus" + std::to_string(yamlIndex) +
                                                 "_" + std::to_string(talkIndex);
                if (ImGui::BeginCombo(focusComboId.c_str(), focusPreview.c_str())) {
                    const bool noFocusSelected = !hasCurrentFocus;
                    if (ImGui::Selectable("フォーカスなし", noFocusSelected)) {
                        npc->ClearTalkCameraFocusTarget(talkIndex);
                    }

                    ImGui::Separator();
                    for (const StageActorInstance& candidate : talkFocusCandidates) {
                        std::string candidateLabel =
                            StageActorQuery::GetTypeLabel(candidate.ref) + " / " + candidate.ref.label;
                        if (const NPC* targetNPC = dynamic_cast<const NPC*>(candidate.actor);
                            targetNPC && !targetNPC->GetName().empty()) {
                            candidateLabel += " (" + targetNPC->GetName() + ")";
                        }
                        candidateLabel += "##talkFocusCandidate" + candidate.ref.sequenceName +
                                          std::to_string(candidate.ref.yamlIndex) + "_" + std::to_string(talkIndex);

                        const bool selected = hasCurrentFocus && currentFocusSequence == candidate.ref.sequenceName &&
                                              currentFocusIndex == candidate.ref.yamlIndex;
                        if (ImGui::Selectable(candidateLabel.c_str(), selected)) {
                            npc->SetTalkCameraFocusTarget(talkIndex, candidate.ref.sequenceName,
                                                          candidate.ref.yamlIndex);
                        }
                    }
                    ImGui::EndCombo();
                }
                ImGui::TextDisabled("設定した会話が表示された間だけ、選択対象へカメラが滑らかに移動します。");
                if (talkTexts.size() > 1 && ImGui::Button(("この会話を削除##placedNPCTalkDelete" +
                                                           std::to_string(yamlIndex) + "_" + std::to_string(talkIndex))
                                                              .c_str())) {
                    npc->RemoveTalkText(talkIndex);
                    break;
                }
            }

            if (ImGui::Button(("会話を追加##placedNPC" + std::to_string(yamlIndex)).c_str())) {
                npc->AddTalkTexts("");
            }
            ImGui::TextDisabled("同じクリア条件の会話が1セットとして順番に表示されます。複数条件を満たす場合は数字が最"
                                "大のステージ条件を使います。");
            if (proximityMessageMode != static_cast<int>(NPCProximityMessageMode::Disabled)) {
                ImGui::TextDisabled("同じ条件に複数ページある場合、最後のページに設定した頭上一言を優先します。");
            }
            ImGui::TextDisabled("変更後、左側の「保存する」でステージへ保存してください。");
        }
    }
