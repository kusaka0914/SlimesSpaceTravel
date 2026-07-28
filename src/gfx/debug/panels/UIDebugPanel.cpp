#include "gfx/debug/panels/UIDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
template <std::size_t Size>
bool EditString(const char* label, std::string& value, std::array<char, Size>& buffer)
{
    std::snprintf(buffer.data(), buffer.size(), "%s", value.c_str());
    if (!ImGui::InputText(label, buffer.data(), buffer.size())) {
        return false;
    }

    value = buffer.data();
    return true;
}

std::string ToLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
}

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

    if (ImGui::BeginTabBar("UIEditorTabs")) {
        if (ImGui::BeginTabItem("追加UIエディタ")) {
            DrawCustomUIEditor(uiLoadSystem);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("既存画像の配置")) {
            DrawTextures(uiLoadSystem);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("既存テキストの配置")) {
            DrawTexts(uiLoadSystem);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

void UIDebugPanel::DrawCustomUIEditor(UILoadSystem* uiLoadSystem)
{
    bool previewEnabled = mContext.uiRenderer->GetCustomUIPreviewEnabled();
    if (ImGui::Checkbox("エディタ中は全要素をプレビュー", &previewEnabled)) {
        mContext.uiRenderer->SetCustomUIPreviewEnabled(previewEnabled);
    }

    ImGui::SameLine();
    if (ImGui::Button("保存")) {
        mStatusMessage = uiLoadSystem->SaveCustomUI() ? "custom_ui.yamlへ保存しました" : "保存に失敗しました";
    }

    ImGui::SameLine();
    if (ImGui::Button("再読込")) {
        if (uiLoadSystem->LoadCustomUI()) {
            mSelectedCustomElement = -1;
            for (const auto& element : uiLoadSystem->GetCustomElements()) {
                if (element.type == UILoadSystem::CustomElementType::Image) {
                    mContext.uiRenderer->RegisterCustomUITexture(element.texturePath);
                }
            }
            mStatusMessage = "custom_ui.yamlを再読込しました";
        } else {
            mStatusMessage = "再読込に失敗しました";
        }
    }

    if (!mStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mStatusMessage.c_str());
    }

    ImGui::Separator();
    DrawCustomElementList(uiLoadSystem);
    ImGui::SameLine();
    DrawCustomElementInspector(uiLoadSystem);
}

void UIDebugPanel::DrawCustomElementList(UILoadSystem* uiLoadSystem)
{
    ImGui::BeginChild("CustomUIElementList", ImVec2(230.0f, 0.0f), true);
    ImGui::TextUnformatted("要素");

    const char* elementTypes[] = {"テキスト", "画像", "パネル"};
    ImGui::Combo("種類", &mNewElementType, elementTypes, IM_ARRAYSIZE(elementTypes));
    ImGui::InputText("画面名", mNewScreen.data(), mNewScreen.size());
    ImGui::InputText("ID", mNewId.data(), mNewId.size());

    if (ImGui::Button("新規追加", ImVec2(-1.0f, 0.0f))) {
        const auto type = static_cast<UILoadSystem::CustomElementType>(mNewElementType);
        mSelectedCustomElement = static_cast<int>(
            uiLoadSystem->AddCustomElement(type, mNewScreen.data(), mNewId.data()));
        mStatusMessage = "要素を追加しました（保存はまだです）";
    }

    ImGui::Separator();

    const auto& elements = uiLoadSystem->GetCustomElements();
    for (std::size_t i = 0; i < elements.size(); ++i) {
        const auto& element = elements[i];
        const std::string label =
            element.screen + "." + element.id + " [" + UILoadSystem::CustomElementTypeToString(element.type) +
            "]##custom" + std::to_string(i);

        if (ImGui::Selectable(label.c_str(), mSelectedCustomElement == static_cast<int>(i))) {
            mSelectedCustomElement = static_cast<int>(i);
        }
    }

    ImGui::EndChild();
}

void UIDebugPanel::DrawCustomElementInspector(UILoadSystem* uiLoadSystem)
{
    ImGui::BeginChild("CustomUIElementInspector", ImVec2(0.0f, 0.0f), true);

    auto& elements = uiLoadSystem->GetCustomElements();
    if (mSelectedCustomElement < 0 ||
        mSelectedCustomElement >= static_cast<int>(elements.size())) {
        ImGui::TextWrapped("左側でテキスト・画像・パネルを追加するか、編集する要素を選んでください。");
        ImGui::EndChild();
        return;
    }

    auto& element = elements[static_cast<std::size_t>(mSelectedCustomElement)];
    std::array<char, 128> screenBuffer = {};
    std::array<char, 128> idBuffer = {};
    std::array<char, 2048> textBuffer = {};

    ImGui::Text("編集: %s.%s", element.screen.c_str(), element.id.c_str());
    EditString("画面名##selected", element.screen, screenBuffer);
    EditString("ID##selected", element.id, idBuffer);

    const bool hasDuplicateKey = std::any_of(
        elements.begin(),
        elements.end(),
        [&](const UILoadSystem::CustomElement& other) {
            return &other != &element && other.screen == element.screen && other.id == element.id;
        });
    if (hasDuplicateKey) {
        ImGui::TextColored(
            ImVec4(1.0f, 0.35f, 0.25f, 1.0f),
            "同じ画面名とIDが存在します。コード制御用に別のIDへ変更してください。");
    }

    ImGui::Checkbox("ゲーム開始時から表示", &element.visibleByDefault);
    ImGui::Checkbox("中心座標を基準にする", &element.centerBased);
    ImGui::InputInt("重なり順", &element.zOrder);

    ImGui::SeparatorText("配置（すべて画面横幅に対する比率）");
    ImGui::SliderFloat("X", &element.xRatio, -0.5f, 1.5f, "%.4f");
    ImGui::SliderFloat("Y", &element.yRatio, -0.25f, 1.0f, "%.4f");

    if (element.type == UILoadSystem::CustomElementType::Text) {
        ImGui::SliderFloat("文字サイズ", &element.textScaleRatio, 0.00005f, 0.003f, "%.7f");
        ImGui::ColorEdit4("文字色", element.color.data());

        std::snprintf(textBuffer.data(), textBuffer.size(), "%s", element.text.c_str());
        if (ImGui::InputTextMultiline(
                "内容",
                textBuffer.data(),
                textBuffer.size(),
                ImVec2(-1.0f, 90.0f))) {
            element.text = textBuffer.data();
        }
    } else {
        ImGui::SliderFloat("幅", &element.widthRatio, 0.001f, 1.5f, "%.4f");
        ImGui::SliderFloat("高さ", &element.heightRatio, 0.001f, 1.5f, "%.4f");

        if (element.type == UILoadSystem::CustomElementType::Panel) {
            ImGui::ColorEdit4("色", element.color.data());
        } else {
            ImGui::Checkbox("画像を上下反転して補正", &element.flipVertical);
            DrawAssetPicker(element);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("この要素を削除")) {
        uiLoadSystem->RemoveCustomElement(static_cast<std::size_t>(mSelectedCustomElement));
        if (mSelectedCustomElement >= static_cast<int>(uiLoadSystem->GetCustomElements().size())) {
            mSelectedCustomElement = static_cast<int>(uiLoadSystem->GetCustomElements().size()) - 1;
        }
        mStatusMessage = "要素を削除しました（保存はまだです）";
    }

    ImGui::EndChild();
}

void UIDebugPanel::DrawAssetPicker(UILoadSystem::CustomElement& element)
{
    if (!mTextureAssetsScanned) {
        RefreshTextureAssets();
    }

    ImGui::SeparatorText("画像アセット");
    ImGui::TextWrapped("選択中: %s", element.texturePath.empty() ? "なし" : element.texturePath.c_str());
    ImGui::InputTextWithHint(
        "##assetFilter",
        "ファイル名で絞り込み",
        mAssetFilter.data(),
        mAssetFilter.size());
    ImGui::SameLine();
    if (ImGui::Button("更新")) {
        RefreshTextureAssets();
    }

    ImGui::BeginChild("TextureAssetPicker", ImVec2(0.0f, 180.0f), true);
    const std::string filter = ToLower(mAssetFilter.data());

    for (const std::string& asset : mTextureAssets) {
        if (!filter.empty() && ToLower(asset).find(filter) == std::string::npos) {
            continue;
        }

        if (ImGui::Selectable(asset.c_str(), element.texturePath == asset)) {
            element.texturePath = asset;
            element.flipVertical = asset != "textures/guard.png";
            if (!mContext.uiRenderer->RegisterCustomUITexture(asset)) {
                mStatusMessage = "画像の読込に失敗しました: " + asset;
            }
        }
    }
    ImGui::EndChild();

    if (!element.texturePath.empty()) {
        mContext.uiRenderer->RegisterCustomUITexture(element.texturePath);
        const GLuint texture = mContext.uiRenderer->GetCustomUITextureHandle(element.texturePath);
        if (texture != 0) {
            const ImVec2 uv0 = element.flipVertical ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
            const ImVec2 uv1 = element.flipVertical ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f);
            ImGui::TextUnformatted("プレビュー");
            ImGui::Image(
                static_cast<ImTextureID>(texture),
                ImVec2(160.0f, 160.0f),
                uv0,
                uv1);
        }
    }
}

void UIDebugPanel::RefreshTextureAssets()
{
    mTextureAssets.clear();
    mTextureAssetsScanned = true;

    const std::filesystem::path assetsRoot = "../assets";
    const std::filesystem::path textureRoot = assetsRoot / "textures";
    std::error_code error;

    if (!std::filesystem::is_directory(textureRoot, error)) {
        mStatusMessage = "assets/texturesが見つかりません";
        return;
    }

    for (std::filesystem::recursive_directory_iterator it(textureRoot, error), end;
         it != end && !error;
         it.increment(error)) {
        if (!it->is_regular_file(error)) {
            continue;
        }

        const std::string extension = ToLower(it->path().extension().string());
        if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" &&
            extension != ".bmp" && extension != ".tga") {
            continue;
        }

        const std::filesystem::path relative = std::filesystem::relative(it->path(), assetsRoot, error);
        if (!error) {
            mTextureAssets.emplace_back(relative.generic_string());
        }
    }

    std::sort(mTextureAssets.begin(), mTextureAssets.end());
    mStatusMessage = std::to_string(mTextureAssets.size()) + "個の画像を検出しました";
}

void UIDebugPanel::DrawTextures(UILoadSystem* uiLoadSystem)
{
    auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
    std::vector<std::string> keys;
    keys.reserve(textureInfos.size());

    for (const auto& pair : textureInfos) {
        keys.emplace_back(pair.first);
    }
    std::sort(keys.begin(), keys.end());

    if (ImGui::Button("既存UIの配置を保存")) {
        mStatusMessage =
            uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml") ? "ui.yamlへ保存しました" : "保存に失敗しました";
    }

    for (const std::string& key : keys) {
        auto& info = textureInfos[key];
        const std::string treeLabel = GetDisplayName(key) + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::Text("ID: %s", key.c_str());
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
    auto& textInfos = uiLoadSystem->GetEditableTextInfos();
    std::vector<std::string> keys;
    keys.reserve(textInfos.size());

    for (const auto& pair : textInfos) {
        keys.emplace_back(pair.first);
    }
    std::sort(keys.begin(), keys.end());

    if (ImGui::Button("既存UIの配置を保存")) {
        mStatusMessage =
            uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml") ? "ui.yamlへ保存しました" : "保存に失敗しました";
    }

    for (const std::string& key : keys) {
        auto& info = textInfos[key];
        const std::string treeLabel = GetDisplayName(key) + "##" + key;

        if (ImGui::TreeNode(treeLabel.c_str())) {
            ImGui::Text("ID: %s", key.c_str());
            ImGui::SliderFloat("X比率", &info.xRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("Y比率", &info.yRatio, 0.0f, 1.0f, "%.4f");
            ImGui::SliderFloat("文字サイズ比率", &info.scaleRatio, 0.0f, 0.005f, "%.7f");
            if (key == "state.talkText") {
                ImGui::SliderFloat(
                    "ルビサイズ倍率（全会話）",
                    &info.rubyScaleRatio,
                    0.2f,
                    1.0f,
                    "%.2f");
                ImGui::SliderFloat(
                    "漢字とルビの間隔（全会話）",
                    &info.rubyGapRatio,
                    -0.5f,
                    2.0f,
                    "%.2f");
            }

            for (const std::string& text : info.texts) {
                ImGui::BulletText("%s", text.c_str());
            }
            ImGui::TreePop();
        }
    }
}

std::string UIDebugPanel::GetDisplayName(const std::string& key) const
{
    static const std::unordered_map<std::string, std::string> displayNames = {
        {"title.bgTexture", "タイトル背景画像"},
        {"opening.bgTexture", "オープニング背景画像"},
        {"gameOver.gameOverText", "ゲームオーバー文字"},
        {"default.hpTexture", "HP UI"},
        {"default.jewelTexture", "ジュエル UI"},
        {"state.stageClearText", "ステージクリア文字"},
        {"state.loadingTexture", "ロード画面画像"},
        {"state.talkBgTexture", "会話背景画像"},
        {"state.talkText", "会話本文"},
    };

    const auto it = displayNames.find(key);
    return it != displayNames.end() ? it->second : key;
}
