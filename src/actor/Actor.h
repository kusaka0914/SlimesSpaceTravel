#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

class Game;
class Component;
class Planet;

class Actor {
public:
    Actor(Game* game);
    virtual ~Actor();

    virtual void Initialize();

    void Update(float deltaTime);
    virtual void UpdateActor(float deltaTime);

    void ProcessInput();
    virtual void ProcessActor();

    void UpdateUpVec();

    void AddComponent(std::unique_ptr<Component> component);
    void RemoveComponent(std::unique_ptr<Component> component);

    void SetIsActive(bool isActive) { mIsActive = isActive; }

    void SetRadius(float radius) { mRadius = radius; }
    void SetFacingYaw(float facingYaw)
    {
        mFacingYaw = facingYaw;
        UpdateDirectionVectors();
    }

    void SetPos(const glm::vec3& pos) { mPos = pos; }
    void SetUpVec(const glm::vec3& upVec)
    {
        mUpVec = upVec;
        UpdateDirectionVectors();
    }
    void SetScale(const glm::vec3& scale) { mScale = scale; }

    void SetModelPath(const std::string& modelPath) { mModelPath = modelPath; }

    void SetCurrentPlanet(Planet* currentPlanet) { mCurrentPlanet = currentPlanet; }
    void SetMeshes(std::vector<struct LoadedMesh>* Meshes) { mMeshes = Meshes; }

    bool GetIsActive() const { return mIsActive; }

    float GetRadius() const { return mRadius; }
    float GetFacingYaw() const { return mFacingYaw; }

    const glm::vec3& GetPos() const { return mPos; }
    const glm::vec3& GetUpVec() const { return mUpVec; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }
    const glm::vec3& GetLeftVec() const { return mLeftVec; }
    glm::vec3 GetRightVec() const { return -mLeftVec; }
    const glm::vec3& GetScale() const { return mScale; }

    const std::string& GetModelPath() const { return mModelPath; }

    Game* GetGame() const { return mGame; }
    Planet* GetCurrentPlanet() const { return mCurrentPlanet; }
    std::vector<struct LoadedMesh>* GetMeshes() const { return mMeshes; }

private:
    glm::vec3 GetAverageNormal();
    bool CastRay(const glm::vec3& offset, glm::vec3& outNormal, const btCollisionObject*& outObj);

protected:
    void UpdateDirectionVectors();
    virtual bool ShouldUpdateUpVecEveryFrame() const { return false; }
    virtual void OnUpVecUpdateFailed();
    void UpdateFallbackUpVec();
    virtual bool CheckDotAngleSteep(const glm::vec3& hitNormal, const glm::vec3& up) const { return false; };
    virtual void OnCastSucceeded(){};

protected:
    bool mIsActive;
    bool mIsUpVecInitialized;

    float mRadius;
    float mFacingYaw;

    glm::vec3 mPos;
    glm::vec3 mUpVec;
    glm::vec3 mForwardVec{0.0f, 0.0f, 1.0f};
    glm::vec3 mLeftVec{-1.0f, 0.0f, 0.0f};
    glm::vec3 mScale;

    std::string mModelPath;

    Game* mGame;
    Planet* mCurrentPlanet;
    std::vector<std::unique_ptr<Component>> mComponents;
    std::vector<struct LoadedMesh>* mMeshes;
};