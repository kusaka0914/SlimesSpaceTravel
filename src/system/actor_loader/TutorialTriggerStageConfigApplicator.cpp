#include "system/actor_loader/TutorialTriggerStageConfigApplicator.h"

#include "actor/TutorialTrigger.h"
#include "system/actor_loader/NpcTalkStageConfigApplicator.h"

#include <algorithm>
#include <string>

void ApplyTutorialTriggerStageConfig(TutorialTrigger& trigger, const YAML::Node& triggerNode)
{
    trigger.SetTutorialId(triggerNode["tutorialId"] ? triggerNode["tutorialId"].as<std::string>() : std::string());
    trigger.SetRequiredCompletedTutorialId(triggerNode["requiredCompletedTutorialId"]
                                               ? triggerNode["requiredCompletedTutorialId"].as<std::string>()
                                               : std::string());

    if (triggerNode["talkTexts"] && triggerNode["talkTexts"].IsSequence()) {
        for (const YAML::Node& textNode : triggerNode["talkTexts"]) {
            trigger.AddTalkTexts(textNode.as<std::string>());
        }
    }
    if (trigger.GetTalkTexts().empty()) {
        trigger.AddTalkTexts("");
    }

    ApplyNpcTalkAdvanceConditions(trigger, triggerNode);

    ApplyNpcTalkPageConfig(trigger, triggerNode);
}

void ApplyTutorialTriggerLegacyScale(TutorialTrigger& trigger, const YAML::Node& triggerNode)
{
    if ((!triggerNode["scale"] || !triggerNode["scale"].IsSequence()) && triggerNode["radius"]) {
        const float legacyRadius = std::max(0.01f, triggerNode["radius"].as<float>());
        trigger.SetScale(glm::vec3(legacyRadius));
    }
}
