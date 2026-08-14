#include "gfx/debug/panels/UIDebugPanel.h"

#include "gfx/UIRenderer.h"
#include "gfx/debug/assets/EditorAssetCatalog.h"
#include "gfx/debug/assets/EditorAssetDragDrop.h"
#include "imgui.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <map>
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

std::string ExtractScreenId(const std::string& elementKey)
{
    const std::size_t separatorIndex = elementKey.find('.');
    return separatorIndex == std::string::npos
               ? std::string()
               : elementKey.substr(0, separatorIndex);
}

std::string ExtractElementId(const std::string& elementKey)
{
    const std::size_t separatorIndex = elementKey.find('.');
    return separatorIndex == std::string::npos
               ? elementKey
               : elementKey.substr(separatorIndex + 1);
}
}

UIDebugPanel::UIDebugPanel(DebugEditorContext& context)
    : DebugPanel(context),
      mCanvasEditor(context)
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

    DrawUIEditor(uiLoadSystem);
}

void UIDebugPanel::DrawUIEditor(UILoadSystem* uiLoadSystem)
{
    switch (mCanvasEditor.GetPrimarySelectionSource()) {
    case UICanvasEditorController::SelectionSource::Custom:
        mSelectedElementSource = SelectedElementSource::Custom;
        mSelectedExistingElementKey.clear();
        break;
    case UICanvasEditorController::SelectionSource::ExistingTexture:
        mSelectedElementSource = SelectedElementSource::ExistingTexture;
        mSelectedExistingElementKey =
            mCanvasEditor.GetPrimaryExistingKey();
        break;
    case UICanvasEditorController::SelectionSource::ExistingText:
        mSelectedElementSource = SelectedElementSource::ExistingText;
        mSelectedExistingElementKey =
            mCanvasEditor.GetPrimaryExistingKey();
        break;
    case UICanvasEditorController::SelectionSource::None:
        mSelectedElementSource = SelectedElementSource::None;
        mSelectedExistingElementKey.clear();
        break;
    }

    bool previewEnabled = mContext.uiRenderer->GetCustomUIPreviewEnabled();
    if (ImGui::Checkbox("エディタ中は全要素をプレビュー", &previewEnabled)) {
        mContext.uiRenderer->SetCustomUIPreviewEnabled(previewEnabled);
    }

    ImGui::SameLine();
    if (ImGui::Button("すべて保存")) {
        SaveAllUI(uiLoadSystem);
    }

    ImGui::SameLine();
    if (ImGui::Button("すべて再読込")) {
        ReloadAllUI(uiLoadSystem);
    }

    if (!mStatusMessage.empty()) {
        ImGui::SameLine();
        ImGui::TextUnformatted(mStatusMessage.c_str());
    }

    ImGui::Separator();
    DrawCanvasToolbar();
    DrawElementList(uiLoadSystem);
    ImGui::SameLine();
    DrawElementInspector(uiLoadSystem);
    mCanvasEditor.Update(uiLoadSystem, mStatusMessage);
}

void UIDebugPanel::DrawCanvasToolbar()
{
    ImGui::TextUnformatted("画面上のUIをクリックして選択できます。Ctrl/Shiftで複数選択、空き場所ドラッグで範囲選択。");

    const auto drawOperationButton =
        [this](
            const char* label,
            UICanvasEditorController::Operation operation) {
            const bool selected = mCanvasEditor.GetOperation() == operation;
            if (selected) {
                ImGui::PushStyleColor(
                    ImGuiCol_Button,
                    ImVec4(0.85f, 0.48f, 0.12f, 1.0f));
            }

            if (ImGui::Button(label)) {
                mCanvasEditor.SetOperation(operation);
            }

            if (selected) {
                ImGui::PopStyleColor();
            }
        };

    drawOperationButton(
        "移動 (E)",
        UICanvasEditorController::Operation::Translate);
    ImGui::SameLine();
    drawOperationButton(
        "回転 (R)",
        UICanvasEditorController::Operation::Rotate);
    ImGui::SameLine();
    drawOperationButton(
        "拡縮 (T)",
        UICanvasEditorController::Operation::Scale);
    ImGui::SameLine();
    ImGui::Text(
        "選択: %zu  |  Ctrl+D:複製  Delete:削除  Ctrl+Z:元に戻す",
        mCanvasEditor.GetSelectedCount());
}

void UIDebugPanel::DrawElementList(UILoadSystem* uiLoadSystem)
{
    ImGui::BeginChild("UIElementList", ImVec2(250.0f, 0.0f), true);
    ImGui::TextUnformatted("要素");

    const char* elementTypes[] = {"テキスト", "画像", "パネル"};
    ImGui::Combo("種類", &mNewElementType, elementTypes, IM_ARRAYSIZE(elementTypes));
    ImGui::InputText("画面名", mNewScreen.data(), mNewScreen.size());
    ImGui::InputText("ID", mNewId.data(), mNewId.size());

    if (ImGui::Button("新規追加", ImVec2(-1.0f, 0.0f))) {
        const auto type = static_cast<UILoadSystem::CustomElementType>(mNewElementType);
        const std::size_t addedIndex =
            uiLoadSystem->AddCustomElement(type, mNewScreen.data(), mNewId.data());
        mCanvasEditor.SetSingleSelection(addedIndex);
        mSelectedElementSource = SelectedElementSource::Custom;
        mSelectedExistingElementKey.clear();
        mStatusMessage = "要素を追加しました（保存はまだです）";
    }

    ImGui::Separator();

    struct ScreenElements {
        std::vector<std::size_t> customIndices;
        std::vector<std::string> textureKeys;
        std::vector<std::string> textKeys;
    };

    const auto& customElements = uiLoadSystem->GetCustomElements();
    std::map<std::string, ScreenElements> elementsByScreen;
    for (std::size_t elementIndex = 0;
         elementIndex < customElements.size();
         ++elementIndex) {
        elementsByScreen[customElements[elementIndex].screen]
            .customIndices.push_back(elementIndex);
    }
    for (const auto& [key, textureInfo] : uiLoadSystem->GetEditableTextureInfos()) {
        (void)textureInfo;
        elementsByScreen[ExtractScreenId(key)].textureKeys.push_back(key);
    }
    for (const auto& [key, textInfo] : uiLoadSystem->GetEditableTextInfos()) {
        (void)textInfo;
        elementsByScreen[ExtractScreenId(key)].textKeys.push_back(key);
    }

    for (auto& [screen, screenElements] : elementsByScreen) {
        std::sort(screenElements.textureKeys.begin(), screenElements.textureKeys.end());
        std::sort(screenElements.textKeys.begin(), screenElements.textKeys.end());

        const std::string screenLabel =
            uiLoadSystem->ResolveCustomScreenDisplayName(screen) +
            "##UIScreen:" + screen;
        if (!ImGui::TreeNodeEx(
                screenLabel.c_str(),
                ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }

        ImGui::TextDisabled("画面ID: %s", screen.c_str());
        for (const std::size_t elementIndex : screenElements.customIndices) {
            const UILoadSystem::CustomElement& element =
                customElements[elementIndex];
            const std::string displayName =
                element.displayName.empty()
                    ? element.id
                    : element.displayName;
            const std::string label =
                displayName + " [" +
                UILoadSystem::CustomElementTypeToString(element.type) +
                "]##custom" + std::to_string(elementIndex);

            const bool isSelected = mCanvasEditor.IsSelected(elementIndex);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                const ImGuiIO& io = ImGui::GetIO();
                mCanvasEditor.SelectFromList(
                    elementIndex,
                    io.KeyCtrl || io.KeySuper || io.KeyShift);
                mSelectedElementSource = SelectedElementSource::Custom;
                mSelectedExistingElementKey.clear();
            }
            ImGui::TextDisabled("ID: %s", element.id.c_str());
        }

        for (const std::string& key : screenElements.textureKeys) {
            const std::string label =
                GetDisplayName(key) + " [image]##existingTexture:" + key;
            const bool isSelected =
                mCanvasEditor.IsExistingTextureSelected(key);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                const ImGuiIO& io = ImGui::GetIO();
                mCanvasEditor.SelectExistingTextureFromList(
                    key,
                    io.KeyCtrl || io.KeySuper || io.KeyShift);
                mSelectedElementSource = SelectedElementSource::ExistingTexture;
                mSelectedExistingElementKey = key;
            }
            ImGui::TextDisabled("ID: %s", ExtractElementId(key).c_str());
        }

        for (const std::string& key : screenElements.textKeys) {
            const std::string label =
                GetDisplayName(key) + " [text]##existingText:" + key;
            const bool isSelected =
                mCanvasEditor.IsExistingTextSelected(key);
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                const ImGuiIO& io = ImGui::GetIO();
                mCanvasEditor.SelectExistingTextFromList(
                    key,
                    io.KeyCtrl || io.KeySuper || io.KeyShift);
                mSelectedElementSource = SelectedElementSource::ExistingText;
                mSelectedExistingElementKey = key;
            }
            ImGui::TextDisabled("ID: %s", ExtractElementId(key).c_str());
        }

        ImGui::TreePop();
    }

    ImGui::EndChild();
}

void UIDebugPanel::DrawElementInspector(UILoadSystem* uiLoadSystem)
{
    ImGui::BeginChild("UIElementInspector", ImVec2(0.0f, 0.0f), true);

    switch (mSelectedElementSource) {
    case SelectedElementSource::Custom:
        DrawCustomElementInspector(uiLoadSystem);
        break;
    case SelectedElementSource::ExistingTexture:
        DrawExistingTextureInspector(uiLoadSystem);
        break;
    case SelectedElementSource::ExistingText:
        DrawExistingTextInspector(uiLoadSystem);
        break;
    case SelectedElementSource::None:
        ImGui::TextWrapped(
            "左側でテキスト・画像・パネルを追加するか、編集する要素を選んでください。");
        break;
    }

    ImGui::EndChild();
}

void UIDebugPanel::DrawCustomElementInspector(UILoadSystem* uiLoadSystem)
{

    auto& elements = uiLoadSystem->GetCustomElements();
    const int selectedElementIndex = mCanvasEditor.GetPrimarySelectedIndex();
    if (selectedElementIndex < 0 ||
        selectedElementIndex >= static_cast<int>(elements.size())) {
        ImGui::TextWrapped("左側でテキスト・画像・パネルを追加するか、編集する要素を選んでください。");
        return;
    }

    auto& element = elements[static_cast<std::size_t>(selectedElementIndex)];
    std::array<char, 128> screenBuffer = {};
    std::array<char, 128> screenDisplayNameBuffer = {};
    std::array<char, 128> idBuffer = {};
    std::array<char, 128> displayNameBuffer = {};
    std::array<char, 2048> textBuffer = {};
    std::array<char, 2048> keyboardTextBuffer = {};
    std::array<char, 2048> gameControllerTextBuffer = {};
    std::array<char, 2048> keyboardModifierTextBuffer = {};
    std::array<char, 2048> gameControllerModifierTextBuffer = {};

    ImGui::Text("編集: %s.%s", element.screen.c_str(), element.id.c_str());
    ImGui::SeparatorText("名前とID");
    if (EditString("画面ID##selected", element.screen, screenBuffer)) {
        uiLoadSystem->SetCustomScreenDisplayName(
            element.screen,
            element.screen);
    }

    std::string screenDisplayName =
        uiLoadSystem->ResolveCustomScreenDisplayName(element.screen);
    if (EditString(
            "画面の表示名##selected",
            screenDisplayName,
            screenDisplayNameBuffer)) {
        uiLoadSystem->SetCustomScreenDisplayName(
            element.screen,
            screenDisplayName);
    }

    EditString(
        "要素の表示名##selected",
        element.displayName,
        displayNameBuffer);
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
    ImGui::SliderFloat("回転角度", &element.rotationDegrees, -180.0f, 180.0f, "%.1f°");

    ImGui::SeparatorText("配置（すべて画面横幅に対する比率）");
    ImGui::SliderFloat("X", &element.xRatio, -0.5f, 1.5f, "%.4f");
    ImGui::SliderFloat("Y", &element.yRatio, -0.25f, 1.0f, "%.4f");

    ImGui::SeparatorText("入力デバイス別表示");
    ImGui::Checkbox(
        "キーボード・ゲームパッドで内容を切り替える",
        &element.usesInputDeviceVariants);
    if (element.usesInputDeviceVariants) {
        ImGui::TextDisabled(
            "デバイス用の内容が空の場合は共通の内容を表示します。");
    }

    if (element.type == UILoadSystem::CustomElementType::Text) {
        ImGui::SliderFloat("文字サイズ", &element.textScaleRatio, 0.00005f, 0.003f, "%.7f");
        ImGui::ColorEdit4("文字色", element.color.data());

        ImGui::SeparatorText("文字効果（距離・太さは画面横幅に対する比率）");
        ImGui::Checkbox("影を表示", &element.shadowEnabled);
        if (element.shadowEnabled) {
            ImGui::ColorEdit4("影の色", element.shadowColor.data());
            ImGui::SliderFloat(
                "影のX位置",
                &element.shadowOffsetXRatio,
                -0.01f,
                0.01f,
                "%.5f");
            ImGui::SliderFloat(
                "影のY位置",
                &element.shadowOffsetYRatio,
                -0.01f,
                0.01f,
                "%.5f");
        }

        ImGui::Checkbox("輪郭を表示", &element.outlineEnabled);
        if (element.outlineEnabled) {
            ImGui::ColorEdit4("輪郭の色", element.outlineColor.data());
            ImGui::SliderFloat(
                "輪郭の太さ",
                &element.outlineWidthRatio,
                0.0001f,
                0.01f,
                "%.5f");
        }

        std::snprintf(textBuffer.data(), textBuffer.size(), "%s", element.text.c_str());
        if (ImGui::InputTextMultiline(
                "共通テキスト",
                textBuffer.data(),
                textBuffer.size(),
                ImVec2(-1.0f, 90.0f))) {
            element.text = textBuffer.data();
        }
        if (element.usesInputDeviceVariants) {
            std::snprintf(
                keyboardTextBuffer.data(),
                keyboardTextBuffer.size(),
                "%s",
                element.keyboardText.c_str());
            if (ImGui::InputTextMultiline(
                    "キーボード用テキスト",
                    keyboardTextBuffer.data(),
                    keyboardTextBuffer.size(),
                    ImVec2(-1.0f, 70.0f))) {
                element.keyboardText = keyboardTextBuffer.data();
            }

            std::snprintf(
                gameControllerTextBuffer.data(),
                gameControllerTextBuffer.size(),
                "%s",
                element.gameControllerText.c_str());
            if (ImGui::InputTextMultiline(
                    "ゲームパッド用テキスト",
                    gameControllerTextBuffer.data(),
                    gameControllerTextBuffer.size(),
                    ImVec2(-1.0f, 70.0f))) {
                element.gameControllerText =
                    gameControllerTextBuffer.data();
            }
        }

        ImGui::SeparatorText("L系入力を押している間");
        ImGui::TextDisabled(
            "デバイス別表示のチェックに関係なく、L/N入力中に使用します。");
        std::snprintf(
            keyboardModifierTextBuffer.data(),
            keyboardModifierTextBuffer.size(),
            "%s",
            element.keyboardModifierText.c_str());
        if (ImGui::InputTextMultiline(
                "キーボード用（N長押し）",
                keyboardModifierTextBuffer.data(),
                keyboardModifierTextBuffer.size(),
                ImVec2(-1.0f, 70.0f))) {
            element.keyboardModifierText =
                keyboardModifierTextBuffer.data();
        }

        std::snprintf(
            gameControllerModifierTextBuffer.data(),
            gameControllerModifierTextBuffer.size(),
            "%s",
            element.gameControllerModifierText.c_str());
        if (ImGui::InputTextMultiline(
                "ゲームパッド用（L長押し）",
                gameControllerModifierTextBuffer.data(),
                gameControllerModifierTextBuffer.size(),
                ImVec2(-1.0f, 70.0f))) {
            element.gameControllerModifierText =
                gameControllerModifierTextBuffer.data();
        }
    } else {
        ImGui::SliderFloat("幅", &element.widthRatio, 0.001f, 1.5f, "%.4f");
        ImGui::SliderFloat("高さ", &element.heightRatio, 0.001f, 1.5f, "%.4f");

        if (element.type == UILoadSystem::CustomElementType::Panel) {
            ImGui::ColorEdit4("色", element.color.data());
        } else {
            std::string* editedTexturePath = &element.texturePath;
            bool* editedFlipVertical = &element.flipVertical;
            const char* imageSectionLabel = "共通画像アセット";
            if (element.usesInputDeviceVariants) {
                const char* variantLabels[] = {
                    "共通（未設定時の代替）",
                    "キーボード用",
                    "ゲームパッド用",
                };
                ImGui::Combo(
                    "編集する画像",
                    &mSelectedInputDeviceImageVariant,
                    variantLabels,
                    IM_ARRAYSIZE(variantLabels));
                if (mSelectedInputDeviceImageVariant == 1) {
                    editedTexturePath = &element.keyboardTexturePath;
                    editedFlipVertical = &element.keyboardFlipVertical;
                    imageSectionLabel = "キーボード用画像アセット";
                } else if (mSelectedInputDeviceImageVariant == 2) {
                    editedTexturePath = &element.gameControllerTexturePath;
                    editedFlipVertical =
                        &element.gameControllerFlipVertical;
                    imageSectionLabel = "ゲームパッド用画像アセット";
                }
            }
            DrawAssetPicker(
                *editedTexturePath,
                *editedFlipVertical,
                "customUITexture",
                imageSectionLabel);
        }
    }

    ImGui::Separator();
    if (ImGui::Button("この要素を複製")) {
        mCanvasEditor.DuplicateSelected(uiLoadSystem, mStatusMessage);
    }

    ImGui::SameLine();
    if (ImGui::Button("この要素を削除")) {
        mCanvasEditor.DeleteSelected(uiLoadSystem, mStatusMessage);
    }

}

void UIDebugPanel::DrawExistingTextureInspector(UILoadSystem* uiLoadSystem)
{
    auto& textureInfos = uiLoadSystem->GetEditableTextureInfos();
    const auto textureInfoIt = textureInfos.find(mSelectedExistingElementKey);
    if (textureInfoIt == textureInfos.end()) {
        mSelectedElementSource = SelectedElementSource::None;
        mSelectedExistingElementKey.clear();
        ImGui::TextUnformatted("選択した画像UIが見つかりません。");
        return;
    }

    UILoadSystem::TextureInfo& textureInfo = textureInfoIt->second;
    ImGui::Text("編集: %s", mSelectedExistingElementKey.c_str());
    ImGui::TextDisabled(
        "ゲームコードと連携して表示されるUIです。");
    ImGui::SeparatorText("名前とID");
    ImGui::Text("表示名: %s", GetDisplayName(mSelectedExistingElementKey).c_str());
    ImGui::Text("画面ID: %s", ExtractScreenId(mSelectedExistingElementKey).c_str());
    ImGui::Text("要素ID: %s", ExtractElementId(mSelectedExistingElementKey).c_str());

    ImGui::SeparatorText("配置（すべて画面横幅に対する比率）");
    ImGui::SliderFloat("X", &textureInfo.xRatio, -0.5f, 1.5f, "%.4f");
    ImGui::SliderFloat("Y", &textureInfo.yRatio, -0.25f, 1.0f, "%.4f");
    ImGui::SliderFloat("幅", &textureInfo.widthRatio, 0.0f, 1.5f, "%.4f");
    ImGui::SliderFloat("高さ", &textureInfo.heightRatio, 0.0f, 1.5f, "%.4f");
    ImGui::SliderFloat(
        "回転角度",
        &textureInfo.rotationDegrees,
        -180.0f,
        180.0f,
        "%.1f°");

    ImGui::SeparatorText("表示方式");
    ImGui::TextWrapped(
        "画像アセットと表示条件はゲームコードから渡されます。"
        "配置・拡縮・回転はほかのUIと同じ操作で編集できます。");
    DrawCodeBoundElementProtection();
}

void UIDebugPanel::DrawExistingTextInspector(UILoadSystem* uiLoadSystem)
{
    auto& textInfos = uiLoadSystem->GetEditableTextInfos();
    const auto textInfoIt = textInfos.find(mSelectedExistingElementKey);
    if (textInfoIt == textInfos.end()) {
        mSelectedElementSource = SelectedElementSource::None;
        mSelectedExistingElementKey.clear();
        ImGui::TextUnformatted("選択したテキストUIが見つかりません。");
        return;
    }

    UILoadSystem::TextInfo& textInfo = textInfoIt->second;
    ImGui::Text("編集: %s", mSelectedExistingElementKey.c_str());
    ImGui::TextDisabled(
        "ゲームコードと連携して表示されるUIです。");
    ImGui::SeparatorText("名前とID");
    ImGui::Text("表示名: %s", GetDisplayName(mSelectedExistingElementKey).c_str());
    ImGui::Text("画面ID: %s", ExtractScreenId(mSelectedExistingElementKey).c_str());
    ImGui::Text("要素ID: %s", ExtractElementId(mSelectedExistingElementKey).c_str());

    ImGui::SeparatorText("配置（すべて画面横幅に対する比率）");
    ImGui::SliderFloat("X", &textInfo.xRatio, -0.5f, 1.5f, "%.4f");
    ImGui::SliderFloat("Y", &textInfo.yRatio, -0.25f, 1.0f, "%.4f");
    ImGui::SliderFloat(
        "文字サイズ",
        &textInfo.scaleRatio,
        0.00005f,
        0.005f,
        "%.7f");
    ImGui::Checkbox("中心座標を基準にする", &textInfo.centerBased);
    ImGui::SliderFloat(
        "回転角度",
        &textInfo.rotationDegrees,
        -180.0f,
        180.0f,
        "%.1f°");

    if (mSelectedExistingElementKey == "state.talkText") {
        ImGui::SliderFloat(
            "ルビサイズ倍率（全会話）",
            &textInfo.rubyScaleRatio,
            0.2f,
            1.0f,
            "%.2f");
        ImGui::SliderFloat(
            "漢字とルビの間隔（全会話）",
            &textInfo.rubyGapRatio,
            -0.5f,
            2.0f,
            "%.2f");
    }

    if (textInfo.texts.empty()) {
        ImGui::SeparatorText("内容");
        ImGui::TextDisabled("内容は会話やゲーム状態から動的に設定されます。");
        DrawCodeBoundElementProtection();
        return;
    }

    ImGui::SeparatorText("内容");
    for (std::size_t textIndex = 0;
         textIndex < textInfo.texts.size();
         ++textIndex) {
        const std::string& text = textInfo.texts[textIndex];
        std::vector<char> textBuffer(
            std::max<std::size_t>(4096, text.size() + 1),
            '\0');
        std::copy(text.begin(), text.end(), textBuffer.begin());

        const std::string label =
            "内容 " + std::to_string(textIndex + 1) +
            "##existingTextContent:" + std::to_string(textIndex);
        if (ImGui::InputTextMultiline(
                label.c_str(),
                textBuffer.data(),
                textBuffer.size(),
                ImVec2(-1.0f, 70.0f))) {
            if (!uiLoadSystem->UpdateTextInfoContent(
                    mSelectedExistingElementKey,
                    textIndex,
                    textBuffer.data())) {
                mStatusMessage = "テキスト内容の更新に失敗しました";
            }
        }
    }
    DrawCodeBoundElementProtection();
}

void UIDebugPanel::DrawCodeBoundElementProtection()
{
    ImGui::Separator();
    ImGui::BeginDisabled();
    ImGui::Button("この要素を複製");
    ImGui::SameLine();
    ImGui::Button("この要素を削除");
    ImGui::EndDisabled();
    ImGui::TextDisabled(
        "ゲームコードがIDを参照するUIのため、複製と削除は保護されています。");
}

void UIDebugPanel::DrawAssetPicker(
    std::string& texturePath,
    bool& flipVertical,
    const char* widgetId,
    const char* sectionLabel)
{
    if (!mContext.assetCatalog) {
        ImGui::TextDisabled("アセットカタログを利用できません");
        return;
    }
    mContext.assetCatalog->EnsureScanned();
    ImGui::PushID(widgetId);

    ImGui::SeparatorText(sectionLabel);
    ImGui::Checkbox("画像を上下反転して補正", &flipVertical);
    ImGui::TextWrapped("選択中: %s", texturePath.empty() ? "なし" : texturePath.c_str());
    ImGui::Button("画像アセットをここへドロップ", ImVec2(-1.0f, 0.0f));
    std::string droppedTexturePath;
    if (EditorAssetDragDrop::AcceptPath(
            EditorAssetType::Texture,
            droppedTexturePath)) {
        texturePath = droppedTexturePath;
        flipVertical = droppedTexturePath != "textures/guard.png";
        if (!mContext.uiRenderer->RegisterCustomUITexture(
                droppedTexturePath)) {
            mStatusMessage =
                "画像の読み込みに失敗しました: " + droppedTexturePath;
        }
    }
    ImGui::InputTextWithHint(
        "##assetFilter",
        "ファイル名で絞り込み",
        mAssetFilter.data(),
        mAssetFilter.size());
    ImGui::SameLine();
    if (ImGui::Button("更新")) {
        mContext.assetCatalog->Refresh();
    }

    ImGui::BeginChild("TextureAssetPicker", ImVec2(0.0f, 180.0f), true);
    const std::string filter = ToLower(mAssetFilter.data());

    for (const std::string& asset :
         mContext.assetCatalog->GetPaths(EditorAssetType::Texture)) {
        if (!filter.empty() && ToLower(asset).find(filter) == std::string::npos) {
            continue;
        }

        if (ImGui::Selectable(asset.c_str(), texturePath == asset)) {
            texturePath = asset;
            flipVertical = asset != "textures/guard.png";
            if (!mContext.uiRenderer->RegisterCustomUITexture(asset)) {
                mStatusMessage = "画像の読込に失敗しました: " + asset;
            }
        }
    }
    ImGui::EndChild();

    if (!texturePath.empty()) {
        mContext.uiRenderer->RegisterCustomUITexture(texturePath);
        const GLuint texture = mContext.uiRenderer->GetCustomUITextureHandle(texturePath);
        if (texture != 0) {
            const ImVec2 uv0 = flipVertical ? ImVec2(0.0f, 1.0f) : ImVec2(0.0f, 0.0f);
            const ImVec2 uv1 = flipVertical ? ImVec2(1.0f, 0.0f) : ImVec2(1.0f, 1.0f);
            ImGui::TextUnformatted("プレビュー");
            ImGui::Image(
                static_cast<ImTextureID>(texture),
                ImVec2(160.0f, 160.0f),
                uv0,
                uv1);
        }
    }
    ImGui::PopID();
}

void UIDebugPanel::SaveAllUI(UILoadSystem* uiLoadSystem)
{
    const bool savedExistingUI =
        uiLoadSystem->SaveUIInfo("../assets/data/ui/ui.yaml");
    const bool savedCustomUI = uiLoadSystem->SaveCustomUI();

    mStatusMessage = savedExistingUI && savedCustomUI
                         ? "すべてのUIを保存しました"
                         : "一部またはすべてのUI保存に失敗しました";
}

void UIDebugPanel::ReloadAllUI(UILoadSystem* uiLoadSystem)
{
    const bool loadedExistingUI = uiLoadSystem->ReloadUIInfo();
    const bool loadedCustomUI = uiLoadSystem->LoadCustomUI();

    if (loadedCustomUI) {
        for (const auto& element : uiLoadSystem->GetCustomElements()) {
            if (element.type == UILoadSystem::CustomElementType::Image) {
                mContext.uiRenderer->RegisterCustomUITexture(element.texturePath);
            }
        }
    }

    mCanvasEditor.ClearSelection();
    mSelectedElementSource = SelectedElementSource::None;
    mSelectedExistingElementKey.clear();
    mStatusMessage = loadedExistingUI && loadedCustomUI
                         ? "すべてのUIを再読込しました"
                         : "一部またはすべてのUI再読込に失敗しました";
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
    return it != displayNames.end() ? it->second : ExtractElementId(key);
}
