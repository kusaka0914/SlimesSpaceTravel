#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Actor;

class MathUtils {
public:
    float GetYawFromDirection(const glm::vec3& up, const glm::vec3& dir) const;
    glm::mat4 CreateOrient(Actor* actor) const;
    glm::quat CalculateActorOrientationFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const;
    glm::vec3 CalculateActorEditorRotationFromOrientation(Actor* actor, const glm::quat& orientation) const;
    glm::vec3 CalculateActorUpVecFromEditorRotation(Actor* actor, const glm::vec3& rotationRad) const;
    void ApplyActorEditorRotation(Actor* actor) const;
    glm::mat4 CreateBillBoard(const glm::mat4& viewMat, const Actor* actor, float upMargin, float rightMargin,
                              float width, float height) const;
    glm::mat4 CreateBillBoard(const glm::mat4& viewMat, const glm::vec3& centerPos, const glm::vec3& upVec, float width,
                              float height) const;

private:
};
