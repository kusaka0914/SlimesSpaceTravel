#include "gfx/debug/panels/UIDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "imgui.h"
#include "system/UILoadSystem.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

UIDebugPanel::UIDebugPanel(DebugEditorContext& context)
    : DebugPanel(context)
{
}

void UIDebugPanel::Draw()
{
    if (!mContext.game || !mContext.uiRenderer) {
        return;
    }

    UILoadSystem* uiLoadSystem = mContext.uiRenderer->GetUILoadSystem();
    if (!uiLoadSystem) {
        return;
    }

    const char* menus[] = {"画像UI", "テキストUI"};

    ImGui::BeginChild("UIEditorLeft", ImVec2(160, 0), true);

    for (int i = 0; i < IM_ARRAYSIZE(menus); ++i) {
        if (ImGui::Selectable(menus[i], mSelectedMenu == i)) {
            mSelectedMenu = i;
        }
    }

    ImGui::Separator();

    if (ImGui::Button("保存する")) {
        uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml");
    }

    ImGui::EndChild();

    ImGui::SameLine();

    ImGui::BeginChild("UIEditorRight", ImVec2(0, 0), true);

    switch (mSelectedMenu) {
    case 0:
        DrawTextures(uiLoadSystem);
        break;
    case 1:
        DrawTexts(uiLoadSystem);
        break;
    default:
        break;
    }

    ImGui::EndChild();
}

void UIDebugPanel::DrawTextures(UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();

    std::vector<std::string> keys;
    keys.reserve(textureInfos.size());

    for (const auto& pair : textureInfos) {
        keys.emplace_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        UILoadSystem::TextureInfo& info = textureInfos[key];

        const std::string displayName = GetDisplayName(key);
        const std::string treeLabel = displayName + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::SliderFloat("X比率", &info.xRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("Y比率", &info.yRatio, 0.0f, 1.0f, "%.4f");

            ImGui::SliderFloat("幅比率", &info.widthRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("高さ比率", &info.heightRatio, 0.0f, 1.0f, "%.4f");

            ImGui::TreePop();
        }
    }
}

void UIDebugPanel::DrawTexts(UILoadSystem* uiLoadSystem)
{
    if (!uiLoadSystem) {
        return;
    }

    auto& textInfos = uiLoadSystem->GetEditableTextInfos();

    std::vector<std::string> keys;
    keys.reserve(textInfos.size());

    for (const auto& pair : textInfos) {
        keys.emplace_back(pair.first);
    }

    std::sort(keys.begin(), keys.end());

    for (const std::string& key : keys) {
        UILoadSystem::TextInfo& info = textInfos[key];

        const std::string displayName = GetDisplayName(key);
        const std::string treeLabel = displayName + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::Text("ID: %s", key.c_str());

            ImGui::SliderFloat("X比率", &info.xRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("Y比率", &info.yRatio, 0.0f, 1.0f, "%.4f");

            ImGui::SliderFloat("文字スケール比率", &info.scaleRatio, 0.0f, 0.005f, "%.7f");

            if (!info.texts.empty()) {
                ImGui::Separator();
                ImGui::Text("表示テキスト");

                for (const std::string& text : info.texts) {
                    ImGui::BulletText("%s", text.c_str());
                }
            }

            ImGui::TreePop();
        }
    }
}

std::string UIDebugPanel::GetDisplayName(const std::string& key) const
{
    static const std::unordered_map<std::string, std::string> displayNames = {
        {"title.bgTexture", "タイトル背景画像"},
        {"title.startTextForGameController", "タイトル開始テキスト（コントローラー）"},
        {"title.startTextForKeyBoard", "タイトル開始テキスト（キーボード）"},

        {"opening.bgTexture", "オープニング背景画像"},
        {"opening.openingText", "オープニング本文"},
        {"opening.talkWithMotherText", "母との会話"},
        {"opening.talkWithDoctorText", "ドクターとの会話"},

        {"gameOver.gameOverText", "ゲームオーバー文字"},
        {"gameOver.restartTextForGameController", "リスタート文字（コントローラー）"},
        {"gameOver.restartTextForKeyBoard", "リスタート文字（キーボード）"},

        {"default.operationSupportTextForGameController", "操作ガイド（コントローラー）"},
        {"default.operationSupportTextForKeyBoard", "操作ガイド（キーボード）"},
        {"default.operationSupportHiddenText", "操作ガイド非表示中テキスト"},
        {"default.hpTexture", "HPUI"},
        {"default.jewelTexture", "ジュエルUI"},
        {"default.talkableTextForGameController", "会話可能テキスト（コントローラー）"},
        {"default.talkableTextForKeyBoard", "会話可能テキスト（キーボード）"},
        {"default.remainPartsText", "残りパーツ数テキスト"},

        {"state.battleTutorialTextForGameController", "戦闘チュートリアル（コントローラー）"},
        {"state.battleTutorialTextForKeyBoard", "戦闘チュートリアル（キーボード）"},
        {"state.breakTutorialText", "ブレイクチュートリアル"},
        {"state.jewelTutorialTextForGameController", "ジュエルチュートリアル（コントローラー）"},
        {"state.jewelTutorialTextForKeyBoard", "ジュエルチュートリアル（キーボード）"},
        {"state.stageClearText", "ステージクリアテキスト"},
        {"state.loadingText", "ローディング文字"},
        {"state.loadingTexture", "ローディング画面スライム画像"},
        {"state.talkBgTexture", "会話背景画像"},
        {"state.talkText", "会話本文"},
    };

    const auto it = displayNames.find(key);
    if (it != displayNames.end()) {
        return it->second;
    }

    return key;
}