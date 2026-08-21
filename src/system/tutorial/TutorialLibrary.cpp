#include "system/tutorial/TutorialLibrary.h"

#include "system/text/JapaneseRubyGenerator.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <utility>
#include <yaml-cpp/yaml.h>

namespace {
std::string ReadString(
    const YAML::Node& node,
    const char* key,
    const std::string& fallback = {})
{
    return node[key]
               ? node[key].as<std::string>()
               : fallback;
}

float ReadFloat(
    const YAML::Node& node,
    const char* key,
    float fallback)
{
    return node[key]
               ? node[key].as<float>()
               : fallback;
}

bool ReadBool(
    const YAML::Node& node,
    const char* key,
    bool fallback)
{
    return node[key]
               ? node[key].as<bool>()
               : fallback;
}

std::string NormalizeRequestedId(std::string requestedId)
{
    for (char& character : requestedId) {
        const unsigned char unsignedCharacter =
            static_cast<unsigned char>(character);
        if (std::isalnum(unsignedCharacter) ||
            character == '_' || character == '-') {
            character = static_cast<char>(
                std::tolower(unsignedCharacter));
        } else {
            character = '_';
        }
    }

    if (requestedId.empty()) {
        return "tutorial";
    }
    return requestedId;
}

void GenerateRuby(
    const std::string& text,
    std::vector<RubyTextSegment>& outSegments)
{
    outSegments.clear();
    if (text.empty()) {
        return;
    }

    std::string errorMessage;
    JapaneseRubyGenerator::Generate(
        text,
        outSegments,
        errorMessage);
}

TutorialPage CreateAssistBattlePage(
    const std::string& id,
    const std::string& controllerText,
    const std::string& keyboardText)
{
    TutorialPage page;
    page.id = id;
    page.text = controllerText;
    page.controllerText = controllerText;
    page.keyboardText = keyboardText;
    page.video.assetPath = "videos/battle_basic_tutorial.mp4";
    GenerateRuby(page.text, page.rubySegments);
    GenerateRuby(
        page.controllerText,
        page.controllerRubySegments);
    GenerateRuby(page.keyboardText, page.keyboardRubySegments);
    return page;
}

bool LoadRubySegments(
    const YAML::Node& node,
    const std::string& baseText,
    std::vector<RubyTextSegment>& outSegments)
{
    if (!node || !node.IsSequence() || baseText.empty()) {
        return false;
    }

    std::vector<RubyTextSegment> loadedSegments;
    for (const YAML::Node& segmentNode : node) {
        if (!segmentNode.IsMap() || !segmentNode["text"]) {
            return false;
        }
        RubyTextSegment segment;
        segment.text = segmentNode["text"].as<std::string>();
        segment.reading =
            segmentNode["reading"]
                ? segmentNode["reading"].as<std::string>()
                : std::string();
        segment.showsRuby =
            segmentNode["ruby"]
                ? segmentNode["ruby"].as<bool>()
                : !segment.reading.empty();
        loadedSegments.emplace_back(std::move(segment));
    }

    if (JoinRubyBaseText(loadedSegments) != baseText) {
        return false;
    }
    outSegments = std::move(loadedSegments);
    return true;
}

YAML::Node SaveRubySegments(const std::vector<RubyTextSegment>& segments)
{
    YAML::Node node(YAML::NodeType::Sequence);
    for (const RubyTextSegment& segment : segments) {
        YAML::Node segmentNode;
        segmentNode["text"] = segment.text;
        segmentNode["ruby"] = segment.showsRuby;
        if (segment.showsRuby) {
            segmentNode["reading"] = segment.reading;
        }
        node.push_back(segmentNode);
    }
    return node;
}
}

const std::string& TutorialPage::ResolveText(
    bool usesController) const
{
    const std::string& variant =
        usesController ? controllerText : keyboardText;
    return variant.empty() ? text : variant;
}

const std::vector<RubyTextSegment>&
TutorialPage::ResolveRubySegments(bool usesController) const
{
    const std::string& variant =
        usesController ? controllerText : keyboardText;
    if (variant.empty()) {
        return rubySegments;
    }
    return usesController
               ? controllerRubySegments
               : keyboardRubySegments;
}

TutorialLibrary::TutorialLibrary(std::string path)
    : mPath(std::move(path))
{
    Load();
}

bool TutorialLibrary::Load()
{
    mDefinitions.clear();
    mLastError.clear();

    try {
        const YAML::Node root = YAML::LoadFile(mPath);
        const YAML::Node tutorials = root["tutorials"];
        if (!tutorials || !tutorials.IsSequence()) {
            mLastError = "tutorials sequence was not found";
            return false;
        }

        for (const YAML::Node& tutorialNode : tutorials) {
            TutorialDefinition definition;
            definition.id = ReadString(tutorialNode, "id");
            if (definition.id.empty() || Find(definition.id)) {
                continue;
            }

            definition.displayName = ReadString(
                tutorialNode,
                "name",
                definition.id);
            definition.repeatPolicy =
                ParseTutorialRepeatPolicyId(
                    ReadString(
                        tutorialNode,
                        "repeat",
                        "oncePerSession"));
            definition.textXRatio = ReadFloat(
                tutorialNode,
                "textXRatio",
                definition.textXRatio);
            definition.textYRatio = ReadFloat(
                tutorialNode,
                "textYRatio",
                definition.textYRatio);
            definition.textScaleRatio = ReadFloat(
                tutorialNode,
                "textScaleRatio",
                definition.textScaleRatio);
            definition.usesAssistPages = ReadBool(
                tutorialNode,
                "usesAssistPages",
                false);

            const auto loadPages = [this](
                                       const YAML::Node& pages,
                                       std::vector<TutorialPage>& target) {
            if (pages && pages.IsSequence()) {
                for (std::size_t pageIndex = 0;
                     pageIndex < pages.size();
                     ++pageIndex) {
                    const YAML::Node pageNode = pages[pageIndex];
                    TutorialPage page;
                    page.id = ReadString(
                        pageNode,
                        "id",
                        "page_" + std::to_string(pageIndex + 1));
                    page.text = ReadString(pageNode, "text");
                    page.controllerText = ReadString(
                        pageNode,
                        "controllerText");
                    page.keyboardText = ReadString(
                        pageNode,
                        "keyboardText");
                    page.advanceCondition =
                        ParseTutorialAdvanceConditionId(
                            ReadString(
                                pageNode,
                                "advance",
                                "confirm"));

                    const YAML::Node focusNode = pageNode["focus"];
                    if (focusNode && focusNode.IsMap()) {
                        page.focusTarget.sequenceName =
                            ReadString(focusNode, "sequence");
                        page.focusTarget.yamlIndex =
                            focusNode["index"]
                                ? focusNode["index"].as<int>()
                                : -1;
                    }

                    const YAML::Node videoNode = pageNode["video"];
                    if (videoNode && videoNode.IsMap()) {
                        page.video.assetPath =
                            ReadString(videoNode, "asset");
                        page.video.xRatio = ReadFloat(
                            videoNode,
                            "xRatio",
                            page.video.xRatio);
                        page.video.yRatio = ReadFloat(
                            videoNode,
                            "yRatio",
                            page.video.yRatio);
                        page.video.widthRatio = ReadFloat(
                            videoNode,
                            "widthRatio",
                            page.video.widthRatio);
                        page.video.heightRatio = ReadFloat(
                            videoNode,
                            "heightRatio",
                            page.video.heightRatio);
                        page.video.rotationDegrees = ReadFloat(
                            videoNode,
                            "rotationDegrees",
                            page.video.rotationDegrees);
                        page.video.shouldLoop = ReadBool(
                            videoNode,
                            "loop",
                            page.video.shouldLoop);
                        page.video.shouldPreserveAspectRatio = ReadBool(
                            videoNode,
                            "preserveAspectRatio",
                            page.video.shouldPreserveAspectRatio);
                        page.video.shouldFlipVertical = ReadBool(
                            videoNode,
                            "flipVertical",
                            page.video.shouldFlipVertical);
                    }

                    RegeneratePageRuby(page);
                    const YAML::Node rubyNode = pageNode["ruby"];
                    if (rubyNode && rubyNode.IsMap()) {
                        LoadRubySegments(
                            rubyNode["common"],
                            page.text,
                            page.rubySegments);
                        LoadRubySegments(
                            rubyNode["controller"],
                            page.controllerText,
                            page.controllerRubySegments);
                        LoadRubySegments(
                            rubyNode["keyboard"],
                            page.keyboardText,
                            page.keyboardRubySegments);
                    }
                    target.emplace_back(std::move(page));
                }
            }
            };

            loadPages(tutorialNode["pages"], definition.pages);
            loadPages(
                tutorialNode["assistPages"],
                definition.assistPages);

            // Seed the original battle tutorial once, then keep the pages as
            // ordinary YAML data that can be edited in the debug editor.
            if (definition.id == "battle_basic" &&
                definition.usesAssistPages) {
                if (definition.assistPages.empty()) {
                    definition.assistPages.emplace_back(
                        CreateAssistBattlePage(
                            "assist_repeat_attack",
                            "敵に近づいて、Yを連打しよう！",
                            "敵に近づいて、Jを連打しよう！"));
                    definition.assistPages.emplace_back(
                        CreateAssistBattlePage(
                            "assist_launch",
                            "Yを連打するだけで、敵を打ち上げられるよ！",
                            "Jを連打するだけで、敵を打ち上げられるよ！"));
                }
            }

            mDefinitions.emplace_back(std::move(definition));
        }
    } catch (const YAML::Exception& exception) {
        mLastError = exception.what();
        return false;
    }

    return true;
}

bool TutorialLibrary::Save()
{
    YAML::Node root;
    root["tutorials"] = YAML::Node(YAML::NodeType::Sequence);

    for (const TutorialDefinition& definition : mDefinitions) {
        YAML::Node tutorialNode;
        tutorialNode["id"] = definition.id;
        tutorialNode["name"] = definition.displayName;
        tutorialNode["repeat"] =
            GetTutorialRepeatPolicyId(definition.repeatPolicy);
        tutorialNode["textXRatio"] = definition.textXRatio;
        tutorialNode["textYRatio"] = definition.textYRatio;
        tutorialNode["textScaleRatio"] =
            definition.textScaleRatio;
        if (definition.usesAssistPages) {
            tutorialNode["usesAssistPages"] = true;
        }
        tutorialNode["pages"] =
            YAML::Node(YAML::NodeType::Sequence);

        for (const TutorialPage& page : definition.pages) {
            YAML::Node pageNode;
            pageNode["id"] = page.id;
            pageNode["text"] = page.text;
            if (!page.controllerText.empty()) {
                pageNode["controllerText"] =
                    page.controllerText;
            }
            if (!page.keyboardText.empty()) {
                pageNode["keyboardText"] =
                    page.keyboardText;
            }
            if (!page.text.empty() &&
                JoinRubyBaseText(page.rubySegments) == page.text) {
                pageNode["ruby"]["common"] =
                    SaveRubySegments(page.rubySegments);
            }
            if (!page.controllerText.empty() &&
                JoinRubyBaseText(page.controllerRubySegments) ==
                    page.controllerText) {
                pageNode["ruby"]["controller"] =
                    SaveRubySegments(page.controllerRubySegments);
            }
            if (!page.keyboardText.empty() &&
                JoinRubyBaseText(page.keyboardRubySegments) ==
                    page.keyboardText) {
                pageNode["ruby"]["keyboard"] =
                    SaveRubySegments(page.keyboardRubySegments);
            }
            pageNode["advance"] =
                GetTutorialAdvanceConditionId(
                    page.advanceCondition);

            if (page.focusTarget.IsValid()) {
                pageNode["focus"]["sequence"] =
                    page.focusTarget.sequenceName;
                pageNode["focus"]["index"] =
                    page.focusTarget.yamlIndex;
            }


            if (page.video.IsEnabled()) {
                pageNode["video"]["asset"] =
                    page.video.assetPath;
                pageNode["video"]["xRatio"] =
                    page.video.xRatio;
                pageNode["video"]["yRatio"] =
                    page.video.yRatio;
                pageNode["video"]["widthRatio"] =
                    page.video.widthRatio;
                pageNode["video"]["heightRatio"] =
                    page.video.heightRatio;
                pageNode["video"]["rotationDegrees"] =
                    page.video.rotationDegrees;
                pageNode["video"]["loop"] =
                    page.video.shouldLoop;
                pageNode["video"]["preserveAspectRatio"] =
                    page.video.shouldPreserveAspectRatio;
                pageNode["video"]["flipVertical"] =
                    page.video.shouldFlipVertical;
            }

            tutorialNode["pages"].push_back(pageNode);
        }

        if (definition.usesAssistPages) {
            tutorialNode["assistPages"] =
                YAML::Node(YAML::NodeType::Sequence);
            for (const TutorialPage& page : definition.assistPages) {
                YAML::Node pageNode;
                pageNode["id"] = page.id;
                pageNode["text"] = page.text;
                if (!page.controllerText.empty()) {
                    pageNode["controllerText"] = page.controllerText;
                }
                if (!page.keyboardText.empty()) {
                    pageNode["keyboardText"] = page.keyboardText;
                }
                if (!page.text.empty() &&
                    JoinRubyBaseText(page.rubySegments) == page.text) {
                    pageNode["ruby"]["common"] =
                        SaveRubySegments(page.rubySegments);
                }
                if (!page.controllerText.empty() &&
                    JoinRubyBaseText(page.controllerRubySegments) ==
                        page.controllerText) {
                    pageNode["ruby"]["controller"] =
                        SaveRubySegments(page.controllerRubySegments);
                }
                if (!page.keyboardText.empty() &&
                    JoinRubyBaseText(page.keyboardRubySegments) ==
                        page.keyboardText) {
                    pageNode["ruby"]["keyboard"] =
                        SaveRubySegments(page.keyboardRubySegments);
                }
                pageNode["advance"] = GetTutorialAdvanceConditionId(
                    page.advanceCondition);
                if (page.focusTarget.IsValid()) {
                    pageNode["focus"]["sequence"] =
                        page.focusTarget.sequenceName;
                    pageNode["focus"]["index"] =
                        page.focusTarget.yamlIndex;
                }
                if (page.video.IsEnabled()) {
                    pageNode["video"]["asset"] = page.video.assetPath;
                    pageNode["video"]["xRatio"] = page.video.xRatio;
                    pageNode["video"]["yRatio"] = page.video.yRatio;
                    pageNode["video"]["widthRatio"] = page.video.widthRatio;
                    pageNode["video"]["heightRatio"] = page.video.heightRatio;
                    pageNode["video"]["rotationDegrees"] = page.video.rotationDegrees;
                    pageNode["video"]["loop"] = page.video.shouldLoop;
                    pageNode["video"]["preserveAspectRatio"] = page.video.shouldPreserveAspectRatio;
                    pageNode["video"]["flipVertical"] = page.video.shouldFlipVertical;
                }
                tutorialNode["assistPages"].push_back(pageNode);
            }
        }

        root["tutorials"].push_back(tutorialNode);
    }

    std::ofstream output(mPath);
    if (!output.is_open()) {
        mLastError = "failed to open tutorial yaml for writing";
        return false;
    }

    output << root;
    mLastError.clear();
    return true;
}

TutorialDefinition* TutorialLibrary::Find(
    const std::string& tutorialId)
{
    const auto definitionIt = std::find_if(
        mDefinitions.begin(),
        mDefinitions.end(),
        [&tutorialId](const TutorialDefinition& definition) {
            return definition.id == tutorialId;
        });
    return definitionIt != mDefinitions.end()
               ? &*definitionIt
               : nullptr;
}

const TutorialDefinition* TutorialLibrary::Find(
    const std::string& tutorialId) const
{
    const auto definitionIt = std::find_if(
        mDefinitions.begin(),
        mDefinitions.end(),
        [&tutorialId](const TutorialDefinition& definition) {
            return definition.id == tutorialId;
        });
    return definitionIt != mDefinitions.end()
               ? &*definitionIt
               : nullptr;
}

TutorialDefinition* TutorialLibrary::Add(
    const std::string& requestedId)
{
    TutorialDefinition definition;
    definition.id = MakeUniqueId(requestedId);
    definition.displayName = definition.id;
    definition.pages.emplace_back(
        TutorialPage{"page_1", "新しいチュートリアル"});
    RegeneratePageRuby(definition.pages.front());
    mDefinitions.emplace_back(std::move(definition));
    return &mDefinitions.back();
}

TutorialDefinition* TutorialLibrary::Duplicate(
    const std::string& tutorialId)
{
    const TutorialDefinition* source = Find(tutorialId);
    if (!source) {
        return nullptr;
    }

    TutorialDefinition duplicated = *source;
    duplicated.id = MakeUniqueId(source->id + "_copy");
    duplicated.displayName += " コピー";
    mDefinitions.emplace_back(std::move(duplicated));
    return &mDefinitions.back();
}

bool TutorialLibrary::Remove(const std::string& tutorialId)
{
    const auto definitionIt = std::find_if(
        mDefinitions.begin(),
        mDefinitions.end(),
        [&tutorialId](const TutorialDefinition& definition) {
            return definition.id == tutorialId;
        });
    if (definitionIt == mDefinitions.end()) {
        return false;
    }
    mDefinitions.erase(definitionIt);
    return true;
}

void TutorialLibrary::RegeneratePageRuby(
    TutorialPage& page) const
{
    GenerateRuby(page.text, page.rubySegments);
    GenerateRuby(
        page.controllerText,
        page.controllerRubySegments);
    GenerateRuby(
        page.keyboardText,
        page.keyboardRubySegments);
}

std::string TutorialLibrary::MakeUniqueId(
    const std::string& requestedId) const
{
    const std::string baseId =
        NormalizeRequestedId(requestedId);
    std::string candidateId = baseId;
    int suffix = 2;
    while (Find(candidateId)) {
        candidateId =
            baseId + "_" + std::to_string(suffix);
        ++suffix;
    }
    return candidateId;
}

const char* GetTutorialAdvanceConditionId(
    TutorialAdvanceCondition condition)
{
    switch (condition) {
    case TutorialAdvanceCondition::PlayerSwitch:
        return "playerSwitch";
    case TutorialAdvanceCondition::Jump:
        return "jump";
    case TutorialAdvanceCondition::PlayerSplitMerge:
        return "playerSplitMerge";
    case TutorialAdvanceCondition::Confirm:
    default:
        return "confirm";
    }
}

TutorialAdvanceCondition ParseTutorialAdvanceConditionId(
    const std::string& conditionId)
{
    if (conditionId == "playerSwitch") {
        return TutorialAdvanceCondition::PlayerSwitch;
    }
    if (conditionId == "jump") {
        return TutorialAdvanceCondition::Jump;
    }
    if (conditionId == "playerSplitMerge") {
        return TutorialAdvanceCondition::PlayerSplitMerge;
    }
    return TutorialAdvanceCondition::Confirm;
}

const char* GetTutorialRepeatPolicyId(
    TutorialRepeatPolicy policy)
{
    return policy == TutorialRepeatPolicy::EveryRequest
               ? "everyRequest"
               : "oncePerSession";
}

TutorialRepeatPolicy ParseTutorialRepeatPolicyId(
    const std::string& policyId)
{
    return policyId == "everyRequest"
               ? TutorialRepeatPolicy::EveryRequest
               : TutorialRepeatPolicy::OncePerSession;
}
