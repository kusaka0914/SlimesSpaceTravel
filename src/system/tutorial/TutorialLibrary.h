#pragma once

#include "text/RubyText.h"

#include <string>
#include <vector>

enum class TutorialAdvanceCondition {
    Confirm = 0,
    PlayerSwitch,
    Jump,
    PlayerSplitMerge,
    ReachPressureSwitchSide,
    ApproachPressureSwitch,
    PressPressureSwitch,
    PlayerSplit,
    PlayerMerge
};

enum class TutorialRepeatPolicy { OnceEver = 0, EveryRequest };

struct TutorialFocusTarget {
    std::string sequenceName;
    int yamlIndex = -1;

    bool IsValid() const { return !sequenceName.empty() && yamlIndex >= 0; }
};

struct TutorialVideoSettings {
    std::string assetPath;
    float xRatio = 0.54f;
    float yRatio = 0.03f;
    float widthRatio = 0.38f;
    float heightRatio = 0.21375f;
    float rotationDegrees = 0.0f;
    bool shouldLoop = true;
    bool shouldPreserveAspectRatio = true;
    bool shouldFlipVertical = true;

    bool IsEnabled() const { return !assetPath.empty(); }
};

struct TutorialPage {
    std::string id;
    std::string text;
    std::string controllerText;
    std::string keyboardText;
    std::string objectiveText;
    std::string controllerObjectiveText;
    std::string keyboardObjectiveText;
    std::string objectivePlatformId;
    TutorialAdvanceCondition advanceCondition = TutorialAdvanceCondition::Confirm;
    TutorialFocusTarget focusTarget;
    TutorialVideoSettings video;
    std::vector<RubyTextSegment> rubySegments;
    std::vector<RubyTextSegment> controllerRubySegments;
    std::vector<RubyTextSegment> keyboardRubySegments;

    const std::string& ResolveText(bool usesController) const;
    const std::string& ResolveObjectiveText(bool usesController) const;
    bool HasConversationText(bool usesController) const { return !ResolveText(usesController).empty(); }
    const std::vector<RubyTextSegment>& ResolveRubySegments(bool usesController) const;
};

struct TutorialDefinition {
    std::string id;
    std::string displayName;
    TutorialRepeatPolicy repeatPolicy = TutorialRepeatPolicy::OnceEver;
    float textXRatio = 0.065f;
    float textYRatio = 0.14f;
    float textScaleRatio = 0.000333333f;

    bool usesAssistPages = false;
    std::vector<TutorialPage> pages;
    std::vector<TutorialPage> assistPages;

    const std::vector<TutorialPage>& GetPagesForControlStyle(bool isAssistControlStyle) const
    {
        if (isAssistControlStyle && usesAssistPages && !assistPages.empty()) {
            return assistPages;
        }
        return pages;
    }
};

// チュートリアル定義とページを保持し、YAML入出力・編集・ルビ再生成を提供する。
class TutorialLibrary {
public:
    explicit TutorialLibrary(std::string path = "../assets/data/tutorials/tutorials.yaml");

    // 全定義を読めた場合だけ置き換える。失敗時はfalseとGetLastErrorで理由を返し、現在の定義を維持する。
    bool Load();
    bool Save();

    TutorialDefinition* Find(const std::string& tutorialId);
    const TutorialDefinition* Find(const std::string& tutorialId) const;

    // IDを正規化して重複しない定義を追加する。メモリ上だけを変更し、既存定義への参照は無効になり得る。
    TutorialDefinition* Add(const std::string& requestedId);

    // 指定定義を別IDで複製する。該当なしはnullptr。追加によって既存定義への参照は無効になり得る。
    TutorialDefinition* Duplicate(const std::string& tutorialId);
    bool Remove(const std::string& tutorialId);

    // 共通・コントローラー・キーボード本文のルビを再生成する。生成できない区間は空になる。
    void RegeneratePageRuby(TutorialPage& page) const;

    std::vector<TutorialDefinition>& GetDefinitions() { return mDefinitions; }
    const std::vector<TutorialDefinition>& GetDefinitions() const { return mDefinitions; }
    const std::string& GetLastError() const { return mLastError; }

private:
    std::string MakeUniqueId(const std::string& requestedId) const;

    std::string mPath;
    std::vector<TutorialDefinition> mDefinitions;
    std::string mLastError;
};

const char* GetTutorialAdvanceConditionId(TutorialAdvanceCondition condition);
TutorialAdvanceCondition ParseTutorialAdvanceConditionId(const std::string& conditionId);

const char* GetTutorialRepeatPolicyId(TutorialRepeatPolicy policy);
TutorialRepeatPolicy ParseTutorialRepeatPolicyId(const std::string& policyId);
