#include "gfx/debug/ugc/UGCEditorElementRegistrar.h"

#include "gfx/UIRenderer.h"

#include <algorithm>
#include <string>

UGCEditorElementRegistrar::UGCEditorElementRegistrar(
    DebugEditorContext& context)
    : mContext(context)
{
}

void UGCEditorElementRegistrar::RegisterElements()
{
    if (!mContext.uiRenderer) {
        return;
    }

    UILoadSystem* uiLoadSystem =
        mContext.uiRenderer->GetUILoadSystem();
    if (!uiLoadSystem) {
        return;
    }




    const auto findElement = [&](const std::string& id)
        -> const UILoadSystem::CustomElement* {
        const auto& elements = uiLoadSystem->GetCustomElements();
        const auto it = std::find_if(
            elements.begin(), elements.end(),
            [&](const UILoadSystem::CustomElement& element) {
                return element.screen == "ugc" && element.id == id;
            });
        return it == elements.end() ? nullptr : &*it;
    };
    const auto ensureButtonElement =
        [&](const char* id,
            const char* displayName,
            float xRatio,
            float yRatio) {
            if (findElement(id)) {
                return;
            }

            const std::size_t index = uiLoadSystem->AddCustomElement(
                UILoadSystem::CustomElementType::Panel, "ugc", id);
            UILoadSystem::CustomElement& element =
                uiLoadSystem->GetCustomElements()[index];
            element.displayName = displayName;
            element.visibleByDefault = false;
            element.xRatio = xRatio;
            element.yRatio = yRatio;
            element.widthRatio = 0.026f;
            element.heightRatio = 0.026f;
        };

    const UILoadSystem::CustomElement* presetTools =
        findElement("presetTools");
    const float presetX = presetTools ? presetTools->xRatio : 0.40625f;
    const float presetY = presetTools ? presetTools->yRatio : 0.0f;
    ensureButtonElement("presetPlatform", "足場", presetX, presetY);
    ensureButtonElement("presetEnemy", "敵", presetX + 0.031f, presetY);
    ensureButtonElement("presetPlanet", "惑星", presetX + 0.062f, presetY);
    ensureButtonElement("presetSwitch", "スイッチ", presetX + 0.093f, presetY);
    ensureButtonElement("presetGoal", "ゴール", presetX + 0.124f, presetY);
    ensureButtonElement("presetMoving", "移動足場", presetX + 0.155f, presetY);
    ensureButtonElement("presetFading", "消える足場", presetX + 0.186f, presetY);
    ensureButtonElement("presetAdhesive", "くっつき足場", presetX + 0.217f, presetY);
    ensureButtonElement("presetTwoPlayer", "2人用スイッチ", presetX + 0.248f, presetY);

    const UILoadSystem::CustomElement* quickTools =
        findElement("quickTools");
    const float quickX = quickTools ? quickTools->xRatio : 0.951f;
    const float quickY = quickTools ? quickTools->yRatio : 0.087f;
    ensureButtonElement("eraser", "消しゴム", quickX, quickY);
    ensureButtonElement("undo", "1つ戻す", quickX, quickY + 0.030f);
    ensureButtonElement("redo", "やり直す", quickX, quickY + 0.060f);

    const UILoadSystem::CustomElement* keyboardTools =
        findElement("keyboardTools");
    const float keyboardX = keyboardTools ? keyboardTools->xRatio : 0.00625f;
    const float keyboardY = keyboardTools ? keyboardTools->yRatio : 0.057f;
    ensureButtonElement("layerUp", "上のだん", keyboardX, keyboardY);
    ensureButtonElement("layerDown", "下のだん", keyboardX, keyboardY + 0.030f);
    ensureButtonElement("zoomIn", "近づく", keyboardX, keyboardY + 0.060f);
    ensureButtonElement("zoomOut", "遠ざかる", keyboardX, keyboardY + 0.090f);
    ensureButtonElement("previewView", "下から見る", keyboardX, keyboardY + 0.120f);

    const auto isLegacyToolbarPanel = [](const std::string& id) {
        return id == "presetTools" || id == "quickTools" ||
               id == "keyboardTools";
    };
    std::vector<const UILoadSystem::CustomElement*> editableElements;
    for (const UILoadSystem::CustomElement& element :
         uiLoadSystem->GetCustomElements()) {
        if (element.screen == "ugc" && !isLegacyToolbarPanel(element.id)) {
            editableElements.push_back(&element);
        }
    }
    std::stable_sort(
        editableElements.begin(), editableElements.end(),
        [](const UILoadSystem::CustomElement* left,
           const UILoadSystem::CustomElement* right) {
            return left->zOrder < right->zOrder;
        });
    for (const UILoadSystem::CustomElement* element : editableElements) {
        mContext.uiRenderer->RecordCustomUIElementForEditor(*element);
    }
}

