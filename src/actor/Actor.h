#pragma once

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Component;
class Game;
class Planet;
struct LoadedMesh;
struct LoadedModel;

// エディターで指定した配置の控え。実行中に変化する座標とは区別して保存する。
// has…がfalseの項目は未記録。localPositionは惑星中心からの相対位置（惑星未設定ならワールド位置）。
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

// ゲーム内オブジェクトの共通基盤。変換、Componentの所有、更新と有効状態を管理する。
// Game・Planet・LoadedModelへのポインタは非所有。参照先は利用中に有効であること。
class Actor {
public:
    // 構築・入力・更新
    explicit Actor(Game* game);
    virtual ~Actor();
    virtual void Initialize();

    // deltaTimeは秒。上方向・向きを更新し、UpdateActor、Componentの順に更新する。
    // 間引き時は経過時間を蓄積するため、UpdateActorには複数フレーム分の時間が渡ることがある。
    void Update(float deltaTime);

    // Actor固有の更新フック。外部からは共通処理を含むUpdateを呼ぶ。
    virtual void UpdateActor(float deltaTime);

    // 共通の実行条件を適用する入力の入口。派生クラスはProcessActorを実装する。
    void ProcessInput();
    virtual void ProcessActor();
    Game* GetGame() const { return mGame; }

    // Componentの所有と更新管理
    // 非nullのComponentの所有権を受け取り、GetUpdateOrderの昇順で更新する。
    void AddComponent(std::unique_ptr<Component> component);

    // 登録済みなら破棄し、そのComponentへの参照は無効になる。未登録なら何もしない。
    void RemoveComponent(Component* component);

    // 有効状態とゲーム進行による表示制御
    void SetIsActive(bool isActive) { mIsActive = isActive; }
    void SetIsDebugDisabled(bool isDebugDisabled) { mIsDebugDisabled = isDebugDisabled; }

    // 非負のstageNumを指定すると他方の条件を解除する。負値は指定した側の条件だけを解除する。
    void SetVisibleIfStageCleared(int stageNum);
    void SetHiddenIfStageCleared(int stageNum);
    void SetHiddenWhenRocketAppears(bool shouldHide) { mHiddenWhenRocketAppears = shouldHide; }

    // source単位の要求を記録し、1つでも無効要求があれば停止する。エディター表示中は無視する。
    // sourceの破棄・解除時はClearRuntimeActivationStateで要求を取り除く。
    void SetRuntimeActivationEnabled(const Component* source, bool isEnabled);
    void ClearRuntimeActivationState(const Component* source);
    void SetShouldAffectGravityDirection(bool shouldAffectGravityDirection)
    {
        mShouldAffectGravityDirection = shouldAffectGravityDirection;
    }
    void SetShouldReactToOverheadGravityRay(bool shouldReactToOverheadGravityRay)
    {
        mShouldReactToOverheadGravityRay = shouldReactToOverheadGravityRay;
    }
    void RefreshProgressVisibility();

    // 明示的な有効フラグに、デバッグ・進行条件・Componentからの要求を加味した実効値。
    bool GetIsActive() const
    {
        return mIsActive && !mIsDebugDisabled && IsProgressVisibleForCurrentMode() &&
               IsRuntimeActivationEnabledForCurrentMode();
    }

    // SetIsActiveで指定された値だけを返す。実効値はGetIsActiveを使う。
    bool IsExplicitlyActive() const { return mIsActive; }
    bool IsDebugDisabled() const { return mIsDebugDisabled; }
    int GetVisibleIfStageCleared() const { return mVisibleIfStageCleared; }
    int GetHiddenIfStageCleared() const { return mHiddenIfStageCleared; }
    bool ShouldAffectGravityDirection() const { return mShouldAffectGravityDirection; }
    bool ShouldReactToOverheadGravityRay() const { return mShouldReactToOverheadGravityRay; }
    bool IsProgressVisibilitySatisfied() const;
    bool IsProgressVisibleForCurrentMode() const;
    bool IsRuntimeActivationEnabledForCurrentMode() const;
    bool ShouldHideWhenRocketAppears() const { return mHiddenWhenRocketAppears; }

    // ワールド変換と惑星上の配置
    // 位置・方向はワールド空間。球面配置パラメーターとは独立して保持する。
    void UpdateUpVec();
    void SetRadius(float radius) { mRadius = radius; }

    // facingYawは現在の上方向を基準としたラジアン角。方向ベクトルも同時に更新する。
    void SetFacingYaw(float facingYaw);
    void SetPos(const glm::vec3& pos) { mPos = pos; }
    void SetUpVec(const glm::vec3& upVec);

    // 姿勢を正規化し、方向ベクトルとYawも同期する。長さが1e-6未満の入力は無視する。
    void SetOrientation(const glm::quat& orientation);
    void SetScale(const glm::vec3& scale) { mScale = scale; }
    void SetCurrentPlanet(Planet* currentPlanet) { mCurrentPlanet = currentPlanet; }
    void SetTheta(float theta) { mTheta = theta; }
    void SetPhi(float phi) { mPhi = phi; }
    void SetHeight(float height) { mHeight = height; }
    float GetRadius() const { return mRadius; }
    float GetFacingYaw() const { return mFacingYaw; }
    const glm::vec3& GetPos() const { return mPos; }
    const glm::vec3& GetUpVec() const { return mUpVec; }
    const glm::vec3& GetForwardVec() const { return mForwardVec; }
    const glm::vec3& GetLeftVec() const { return mLeftVec; }
    glm::vec3 GetRightVec() const { return -mLeftVec; }
    const glm::quat& GetOrientation() const { return mOrientation; }
    const glm::vec3& GetScale() const { return mScale; }
    Planet* GetCurrentPlanet() const { return mCurrentPlanet; }
    float GetTheta() const { return mTheta; }
    float GetPhi() const { return mPhi; }
    float GetHeight() const { return mHeight; }

    // thetaはXZ方位、phiは仰角（ラジアン）、heightは惑星半径からの追加距離。
    // パラメーターだけを保存し、ワールド位置は変更しない。
    void SetSphericalPlacement(float theta, float phi, float height)
    {
        mTheta = theta;
        mPhi = phi;
        mHeight = height;
    }

    // モデル・テクスチャ・描画用の値
    void SetTextureTiling(const glm::vec2& textureTiling) { mTextureTiling = textureTiling; }
    void SetTextureOverridePath(const std::string& texturePath) { mTextureOverridePath = texturePath; }
    void SetModelPath(const std::string& modelPath) { mModelPath = modelPath; }

    // モデルの非所有参照を設定し、OnLoadedModelChangedを通知する。nullptrは未設定を表す。
    void SetLoadedModel(const LoadedModel* loadedModel);
    virtual float GetRenderOpacity() const { return 1.0f; }
    virtual bool ShouldRenderSolidWhite() const { return false; }
    virtual glm::vec3 GetRenderPosition() const { return mPos; }
    virtual glm::vec3 GetRenderScale() const { return mScale; }
    const glm::vec2& GetTextureTiling() const { return mTextureTiling; }
    virtual glm::vec2 GetRenderTextureTiling() const { return mTextureTiling; }
    virtual glm::quat GetRenderModelRotationOffset() const { return glm::quat(1.0f, 0.0f, 0.0f, 0.0f); }
    const std::string& GetTextureOverridePath() const { return mTextureOverridePath; }
    virtual const std::string& GetRenderTextureOverridePath() const { return mTextureOverridePath; }
    const std::string& GetModelPath() const { return mModelPath; }
    const LoadedModel* GetLoadedModel() const { return mLoadedModel; }

    // モデル未設定時はnullptr。返すポインタの所有権は移さない。
    const std::vector<LoadedMesh>* GetMeshes() const;

    // スキニング非対応のActorはnullptrを返す。描画側でnull判定すること。
    virtual const std::vector<glm::mat4>* GetSkinningMatrices() const { return nullptr; }
    virtual float GetCollisionScaleMultiplier() const { return 1.0f; }

    // エディター用の配置情報
    void SetEditorRotation(const glm::vec3& rotation) { mEditorRotation = rotation; }
    const glm::vec3& GetEditorRotation() const { return mEditorRotation; }
    int GetStageYamlIndex() const { return mStageYamlIndex; }
    void SetStageYamlIndex(int index) { mStageYamlIndex = index; }
    const std::string& GetStageSequenceName() const { return mStageSequenceName; }
    void SetStageSequenceName(const std::string& sequenceName) { mStageSequenceName = sequenceName; }
    void SetIsEditorSelected(bool isEditorSelected) { mIsEditorSelected = isEditorSelected; }

    // 現在の配置をエディター保存用に記録する。位置・回転・スケールは個別に記録できる。
    void CaptureEditorAuthoredPosition();
    void CaptureEditorAuthoredRotation();
    void CaptureEditorAuthoredScale();
    void ClearEditorAuthoredTransform() { mEditorAuthoredTransform = EditorAuthoredTransform{}; }

    // 記録済みの項目がなければnullptr。返すポインタはこのActorに属する内部データを指す。
    const EditorAuthoredTransform* FindEditorAuthoredTransform() const
    {
        const bool hasAuthoredTransform = mEditorAuthoredTransform.hasPosition ||
                                          mEditorAuthoredTransform.hasRotation || mEditorAuthoredTransform.hasScale;
        return hasAuthoredTransform ? &mEditorAuthoredTransform : nullptr;
    }
    bool GetIsEditorSelected() const { return mIsEditorSelected; }

protected:
    void UpdateDirectionVectors();
    void UpdateOrientationFromDirectionVectors();

    // 0なら毎フレーム更新。正ならUpdateActorとComponentの更新をまとめて間引く。
    virtual float ResolveMinimumUpdateIntervalSeconds() const { return 0.0f; }
    virtual bool ShouldUpdateUpVecEveryFrame() const;
    virtual bool ShouldRebuildDirectionVectorsEveryFrame() const { return true; }
    virtual void OnUpVecUpdateFailed();
    void UpdateFallbackUpVec();
    virtual bool CheckDotAngleSteep(const glm::vec3& hitNormal, const glm::vec3& up) const { return false; }

    // レイが面に当たった時点の通知と、法線の検証を通過した時点の通知を区別する。
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
    std::unordered_map<const Component*, bool> mRuntimeActivationStates;
    const LoadedModel* mLoadedModel;

    glm::vec3 mEditorRotation{0.0f};
    bool mIsEditorSelected = false;
    EditorAuthoredTransform mEditorAuthoredTransform;

private:
    float mDeferredUpdateSeconds = 0.0f;
};
