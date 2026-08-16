#pragma once

#include "Game.h"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Component;
class Planet;
struct LoadedMesh;
struct LoadedModel;

struct EditorAuthoredTransform {
    bool hasPosition = false;
    bool hasRotation = false;
    bool hasScale = false;
    Planet* planet = nullptr;
    glm::vec3 localPosition{0.0f};
    glm::vec3 editorRotation{0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::vec3 upDirection{0.0f, 1.0f, 0.0f};
    float facingYaw = 0.0f;
    float theta = 0.0f;
    float phi = 0.0f;
    float height = 0.0f;
};

class Actor {
public:
    explicit Actor(Game* game);
    virtual ~Actor();

    virtual void Initialize();

    void Update(float deltaTime);
    virtual void UpdateActor(float deltaTime);

    void ProcessInput();
    virtual void ProcessActor();

    void UpdateUpVec();

    void AddComponent(std::unique_ptr<Component> component);
    void RemoveComponent(Component* component);

    void SetIsActive(bool isActive) { mIsActive = isActive; }
    void SetIsDebugDisabled(bool isDebugDisabled)
    {
        mIsDebugDisabled = isDebugDisabled;
    }
    void SetVisibleIfStageCleared(int stageNum);
    void SetHiddenIfStageCleared(int stageNum);
    void SetHiddenWhenRocketAppears(bool shouldHide)
    {
        mHiddenWhenRocketAppears = shouldHide;
    }
    void SetRuntimeActivationEnabled(
        const Component* source,
        bool isEnabled);
    void ClearRuntimeActivationState(const Component* source);
    void SetShouldAffectGravityDirection(bool shouldAffectGravityDirection)
    {
        mShouldAffectGravityDirection = shouldAffectGravityDirection;
    }
    void SetShouldReactToOverheadGravityRay(
        bool shouldReactToOverheadGravityRay)
    {
        mShouldReactToOverheadGravityRay =
            shouldReactToOverheadGravityRay;
    }
    void RefreshProgressVisibility();

    void SetRadius(float radius) { mRadius = radius; }
    void SetFacingYaw(float facingYaw);

    void SetPos(const glm::vec3& pos) { mPos = pos; }
    void SetUpVec(const glm::vec3& upVec);
    void SetOrientation(const glm::quat& orientation);
    void SetScale(const glm::vec3& scale) { mScale = scale; }
    void SetTextureTiling(const glm::vec2& textureTiling) { mTextureTiling = textureTiling; }
    void SetTextureOverridePath(const std::string& texturePath) { mTextureOverridePath = texturePath; }

    void SetModelPath(const std::string& modelPath) { mModelPath = modelPath; }
    void SetLoadedModel(const LoadedModel* loadedModel);

    void SetCurrentPlanet(Planet* currentPlanet) { mCurrentPlanet = currentPlanet; }

    void SetTheta(float theta) { mTheta = theta; }
    void SetPhi(float phi) { mPhi = phi; }
    void SetHeight(float height) { mHeight = height; }

    bool GetIsActive() const
    {
        return mIsActive &&
               !mIsDebugDisabled &&
               IsProgressVisibleForCurrentMode() &&
               IsRuntimeActivationEnabledForCurrentMode();
    }
    bool IsExplicitlyActive() const { return mIsActive; }
    bool IsDebugDisabled() const { return mIsDebugDisabled; }
    virtual float GetRenderOpacity() const { return 1.0f; }
    virtual bool ShouldRenderSolidWhite() const { return false; }
    int GetVisibleIfStageCleared() const { return mVisibleIfStageCleared; }
    int GetHiddenIfStageCleared() const { return mHiddenIfStageCleared; }
    bool ShouldAffectGravityDirection() const { return mShouldAffectGravityDirection; }
    bool ShouldReactToOverheadGravityRay() const
    {
        return mShouldReactToOverheadGravityRay;
    }
    bool IsProgressVisibilitySatisfied() const;
    bool IsProgressVisibleForCurrentMode() const
    {
        return IsProgressVisibilitySatisfied() ||
               (mGame && mGame->GetIsDebugEditorShowing());
    }
    bool IsRuntimeActivationEnabledForCurrentMode() const;
    bool ShouldHideWhenRocketAppears() const
    {
        return mHiddenWhenRocketAppears;
    }

    float GetRadius() const { return mRadius; }
    float GetFacingYaw() const { return mFacingYaw; }

    const glm::vec3& GetPos() const { return mPos; }
    const glm::vec3& GetUpVec() const { return mUpVec; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }
    const glm::vec3& GetLeftVec() const { return mLeftVec; }
    glm::vec3 GetRightVec() const { return -mLeftVec; }
    const glm::quat& GetOrientation() const { return mOrientation; }
    const glm::vec3& GetScale() const { return mScale; }
    const glm::vec2& GetTextureTiling() const { return mTextureTiling; }
    virtual glm::vec2 GetRenderTextureTiling() const
    {
        return mTextureTiling;
    }
    const std::string& GetTextureOverridePath() const { return mTextureOverridePath; }
    virtual const std::string& GetRenderTextureOverridePath() const
    {
        return mTextureOverridePath;
    }

    const std::string& GetModelPath() const { return mModelPath; }
    const LoadedModel* GetLoadedModel() const { return mLoadedModel; }
    const std::vector<LoadedMesh>* GetMeshes() const;

    virtual const std::vector<glm::mat4>* GetSkinningMatrices() const { return nullptr; }
    virtual float GetCollisionScaleMultiplier() const { return 1.0f; }

    Game* GetGame() const { return mGame; }
    Planet* GetCurrentPlanet() const { return mCurrentPlanet; }
    float GetTheta() const { return mTheta; }
    float GetPhi() const { return mPhi; }
    float GetHeight() const { return mHeight; }

    void SetSphericalPlacement(float theta, float phi, float height)
    {
        mTheta = theta;
        mPhi = phi;
        mHeight = height;
    }

    void SetEditorRotation(const glm::vec3& rotation) { mEditorRotation = rotation; }
    const glm::vec3& GetEditorRotation() const { return mEditorRotation; }

    int GetStageYamlIndex() const { return mStageYamlIndex; }
    void SetStageYamlIndex(int index) { mStageYamlIndex = index; }
    const std::string& GetStageSequenceName() const { return mStageSequenceName; }
    void SetStageSequenceName(const std::string& sequenceName) { mStageSequenceName = sequenceName; }

    void SetIsEditorSelected(bool isEditorSelected) { mIsEditorSelected = isEditorSelected; }

    // Captures a transform changed by an editor operation. Runtime movement
    // intentionally does not call this, so stage saves cannot turn an AI or
    // animation position into the next spawn position.
    void CaptureEditorAuthoredPosition();
    void CaptureEditorAuthoredRotation();
    void CaptureEditorAuthoredScale();
    void ClearEditorAuthoredTransform()
    {
        mEditorAuthoredTransform = EditorAuthoredTransform{};
    }
    const EditorAuthoredTransform* FindEditorAuthoredTransform() const
    {
        const bool hasAuthoredTransform =
            mEditorAuthoredTransform.hasPosition ||
            mEditorAuthoredTransform.hasRotation ||
            mEditorAuthoredTransform.hasScale;
        return hasAuthoredTransform
            ? &mEditorAuthoredTransform
            : nullptr;
    }

    bool GetIsEditorSelected() const { return mIsEditorSelected; }

protected:
    void UpdateDirectionVectors();
    void UpdateOrientationFromDirectionVectors();
    virtual float ResolveMinimumUpdateIntervalSeconds() const
    {
        return 0.0f;
    }
    virtual bool ShouldUpdateUpVecEveryFrame() const { return mGame->GetIsDebugMode(); }
    virtual bool ShouldRebuildDirectionVectorsEveryFrame() const { return true; }
    virtual void OnUpVecUpdateFailed();
    void UpdateFallbackUpVec();
    virtual bool CheckDotAngleSteep(const glm::vec3& hitNormal, const glm::vec3& up) const { return false; }
    virtual void OnGroundSurfaceDetected() {}
    virtual void OnCastSucceeded() {}
    virtual void OnLoadedModelChanged() {}

protected:
    bool mIsActive;
    bool mIsDebugDisabled = false;
    bool mStageClearVisibilitySatisfied = true;
    bool mHiddenWhenRocketAppears = false;
    bool mShouldAffectGravityDirection = true;
    bool mShouldReactToOverheadGravityRay = false;
    int mVisibleIfStageCleared = -1;
    int mHiddenIfStageCleared = -1;
    bool mIsUpVecInitialized;

    float mRadius;
    float mFacingYaw;
    float mTheta = 0.0f;
    float mPhi = 0.0f;
    float mHeight = 0.0f;
    int mStageYamlIndex = 0;

    glm::vec3 mPos;
    glm::vec3 mUpVec;
    glm::vec3 mForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 mLeftVec{1.0f, 0.0f, 0.0f};
    glm::quat mOrientation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 mScale;
    glm::vec2 mTextureTiling{1.0f, 1.0f};

    std::string mModelPath;
    std::string mTextureOverridePath;
    std::string mStageSequenceName;

    Game* mGame;
    Planet* mCurrentPlanet;
    std::vector<std::unique_ptr<Component>> mComponents;
    std::unordered_map<const Component*, bool>
        mRuntimeActivationStates;
    const LoadedModel* mLoadedModel;

    glm::vec3 mEditorRotation{0.0f};
    bool mIsEditorSelected = false;
    EditorAuthoredTransform mEditorAuthoredTransform;

private:
    float mDeferredUpdateSeconds = 0.0f;
};
