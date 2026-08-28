#pragma once

#include "text/RubyText.h"

#include <string>
#include <vector>

enum class TutorialAdvanceCondition {
    Confirm = 0,
    PlayerSwitch,
    Jump,
    PlayerSplitMerge
};

enum class TutorialRepeatPolicy {
    OnceEver = 0,
    EveryRequest
};

struct TutorialFocusTarget {
    std::string sequenceName;
    int yamlIndex = -1;

    bool IsValid() const
    {
        return !sequenceName.empty() && yamlIndex >= 0;
    }
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
    TutorialAdvanceCondition advanceCondition =
        TutorialAdvanceCondition::Confirm;
    TutorialFocusTarget focusTarget;
    TutorialVideoSettings video;
    std::vector<RubyTextSegment> rubySegments;
    std::vector<RubyTextSegment> controllerRubySegments;
    std::vector<RubyTextSegment> keyboardRubySegments;

    const std::string& ResolveText(bool usesController) const;
    const std::vector<RubyTextSegment>&
    ResolveRubySegments(bool usesController) const;
};

struct TutorialDefinition {
    std::string id;
    std::string displayName;
    TutorialRepeatPolicy repeatPolicy =
        TutorialRepeatPolicy::OnceEver;
    float textXRatio = 0.065f;
    float textYRatio = 0.14f;
    float textScaleRatio = 0.000333333f;


    bool usesAssistPages = false;
    std::vector<TutorialPage> pages;
    std::vector<TutorialPage> assistPages;

    const std::vector<TutorialPage>& GetPagesForControlStyle(
        bool isAssistControlStyle) const
    {
        if (isAssistControlStyle && usesAssistPages &&
            !assistPages.empty()) {
            return assistPages;
        }
        return pages;
    }
};

class TutorialLibrary {
public:
    explicit TutorialLibrary(
        std::string path =
            "../assets/data/tutorials/tutorials.yaml");

    bool Load();
    bool Save();

    TutorialDefinition* Find(const std::string& tutorialId);
    const TutorialDefinition* Find(
        const std::string& tutorialId) const;

    TutorialDefinition* Add(const std::string& requestedId);
    TutorialDefinition* Duplicate(const std::string& tutorialId);
    bool Remove(const std::string& tutorialId);

    void RegeneratePageRuby(TutorialPage& page) const;

    std::vector<TutorialDefinition>& GetDefinitions()
    {
        return mDefinitions;
    }
    const std::vector<TutorialDefinition>& GetDefinitions() const
    {
        return mDefinitions;
    }
    const std::string& GetLastError() const { return mLastError; }

private:
    std::string MakeUniqueId(const std::string& requestedId) const;

private:
    std::string mPath;
    std::vector<TutorialDefinition> mDefinitions;
    std::string mLastError;
};

const char* GetTutorialAdvanceConditionId(
    TutorialAdvanceCondition condition);
TutorialAdvanceCondition ParseTutorialAdvanceConditionId(
    const std::string& conditionId);

const char* GetTutorialRepeatPolicyId(
    TutorialRepeatPolicy policy);
TutorialRepeatPolicy ParseTutorialRepeatPolicyId(
    const std::string& policyId);
