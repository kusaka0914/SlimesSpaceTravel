#pragma once

#include "gfx/LightingSettings.h"
#include "gfx/Renderer.h"
#include "gfx/render3d/SceneRenderState.h"
#include "text/RubyText.h"

#include <GL/glew.h>
#include <SDL_ttf.h>
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Actor;
class CharacterActor;
class DebugLabelRenderer;
class DirectionalShadowMap;
class EnvironmentDecorationRenderer;
class Enemy;
class CameraSystem;
class FramePerformanceTracker;
class MathUtils;
class MeshLoadSystem;
class PhysicsSystem;
class UILoadSystem;
struct GLFWwindow;
class NPCProximityMessageRenderer;
class Player;
class PlayerEffectRenderer;
class PlanetAtmosphereRenderer;
class ParticleRenderer;
class Planet;
class RenderViewportController;
class SceneObjectRenderer;
class Shader3D;
class VertexArray;

// UGCの編集レイヤー強調を、現在の編集状態に従うか明示指定するかを切り替える。
enum class UGCSceneLayerRenderMode {
    AutomaticallyHighlightEditingLayer,
    HighlightEditingLayer,
    HighlightEditingLayerWithoutDimming,
    ShowAllLayers,
};

// 参照先はRenderer3Dより長く有効に保つ。readSceneは必須で、描画中に有効な現在のシーンを同期取得する。
struct Renderer3DDependencies {
    GLFWwindow& window;
    const MathUtils& math;
    MeshLoadSystem& models;
    const PhysicsSystem& physics;
    const UILoadSystem& uiDefinitions;
    CameraSystem& cameras;
    FramePerformanceTracker& performance;
    std::function<SceneRenderState()> readScene;
};

// 3D描画の窓口。Actor描画と材質設定を担い、視点分割・場面構成・各種演出を専用Rendererへ委譲する。
class Renderer3D : public Renderer {
public:
    explicit Renderer3D(Renderer3DDependencies dependencies);
    ~Renderer3D() override;

    // シェーダー・基本テクスチャ・描画モジュールを生成する。コンストラクタから呼ばれる初期化処理。
    void Initialize();

    // OpenGLコンテキストが有効な間に、影・大気・装飾・viewport・攻撃範囲用のリソースを解放する。
    // 繰り返し呼び出せる。デストラクタからも呼ぶ。
    void Shutdown();

    // 現在のゲーム表示モードに応じてviewportを描画する。タイトル・操作方式選択画面では何もしない。
    void Draw() const;

    // 操作中のPlayerを優先して注目位置を決め、通常ゲーム画面用の影を更新する。
    void RenderGameplayShadowMap() const;

    // ワールド座標の注目位置を中心に、現在ステージの影を更新する。Playerは簡易影のみとし、対象に含めない。
    void RenderShadowMap(const glm::vec3& focusPosition) const;

    // 現在ステージの装飾を準備して描画する。UGCモードでは描画しない。
    void DrawEnvironmentDecorations(const SceneRenderState& scene) const;

    // 指定した視点でシーン・大気・パーティクルを描画する。描画先とviewportは呼び出し側で設定する。
    // viewportPlayerは視点固有の表示に使い、省略時は特定Playerへ限定しない。
    void
    DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos,
              UGCSceneLayerRenderMode ugcLayerRenderMode = UGCSceneLayerRenderMode::AutomaticallyHighlightEditingLayer,
              int ugcEditLayer = 0, const Player* viewportPlayer = nullptr) const;

    Shader3D* GetShader3D() const { return mShader3D; }

    // 未登録ならnullptr／0。取得した基本形状・テクスチャはRenderer側の管理対象。
    VertexArray* FindVertexArray(const std::string& name) const;
    GLuint FindTexture(const std::string& name) const;
    GLuint GetAttackRangeVAO() const { return mAttackRangeVAO; }
    GLuint GetAttackRangeVBO() const { return mAttackRangeVBO; }

    // assets相対パスを読み込み、成功・失敗をキャッシュする。絶対パス・assets外へ遡るパス・読込失敗は0。
    // 戻り値は借用ハンドルで、呼び出し側では解放しない。
    GLuint GetOrLoadTextureOverride(const std::string& assetRelativePath) const;
    LightingSettings& GetLightingSettings() { return mLightingSettings; }
    const LightingSettings& GetLightingSettings() const { return mLightingSettings; }

    // 生成した文字テクスチャは呼び出し側で解放する。失敗時は0。出力寸法は倍率適用後のピクセル数。
    GLuint CreateTextTextureFor3D(const std::string& text, int& outWidth, int& outHeight, const SDL_Color textColor,
                                  float textScale) const
    {
        return CreateTextTexture(text, outWidth, outHeight, textColor, textScale);
    }

    // ルビ込みのテクスチャとピクセル寸法を返す。outBaseTextHeightは本文のみの高さ。
    // 失敗時は0。成功したテクスチャは呼び出し側で解放する。
    GLuint CreateRubyTextTextureFor3D(const std::vector<RubyTextSegment>& segments, int& outWidth, int& outHeight,
                                      int& outBaseTextHeight, const SDL_Color textColor, float rubyScaleRatio,
                                      float rubyGapRatio) const;

    // アルファ合成を有効化して深度書き込みを止める。Endは深度書き込み有効・合成無効にし、以前の状態は保持しない。
    void StartTransparentDraw() const;
    void EndTransparentDraw() const;

    // nullptr・非アクティブ・ほぼ透明なActorを除外してDrawActorへ渡す。useOrientはActorの姿勢を適用するか。
    void TryDrawActor(Actor* actor, bool useOrient = true, const PlayerSoulPresentation& soul = {}) const;

    // Actorのメッシュと材質を描画する。非アクティブ状態は除外せず、プレビューにも使用する。
    // soulは1人プレイの分裂Playerにだけ適用し、輪郭の個体識別とは独立して扱う。
    void DrawActor(Actor* actor, bool useOrient = true, const PlayerSoulPresentation& soul = {}) const;
    void DrawUGCPlacementPreviewActor(Actor* actor) const;

    // アクティブ状態を変えずに指定の不透明度倍率で描画する。opacityは0〜1へ制限する。
    void DrawInactiveActorPreview(Actor* actor, float opacity) const;
    // 光源に依存しない足元の簡易影。Playerは体幅・奥行きに合わせた均一な黒で、実際の支持面へ投影する。
    // Playerの影はジャンプ高度で大きさを変えない。
    void DrawBlobShadow(const CharacterActor* actor) const;

    // 直近のDrawSceneの視錐台に外接球が入るかを返す。視錐台未設定・nullptrの場合は除外せずtrue。
    bool IsActorInsideView(const Actor* actor) const;

    template <class ActorRange> void TryDrawActors(const ActorRange& actors, bool useOrient = true) const
    {
        if (actors.empty()) {
            return;
        }

        for (auto* actor : actors) {
            if (!actor || !actor->GetIsActive() || actor->GetRenderOpacity() <= 0.001f) {
                continue;
            }

            DrawActor(actor, useOrient);
        }
    }

    // ワールド座標の頂点列を指定プリミティブで半透明描画する。現在の3Dシェーダーと視点設定を使う。
    // 線描画時にlineWidthを指定すると対応範囲へ補正し、描画後に元の線幅へ戻す。単位はピクセル。
    void DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode, const glm::vec4& color,
                                 std::optional<float> lineWidth = std::nullopt) const;

    // カメラへ向くモデル行列を返す。位置と寸法はワールド単位。姿勢計算には借用中のMathUtilsを使う。
    glm::mat4 CreateBillboard(const glm::mat4& viewMat, const Actor* actor, float upMargin, float rightMargin,
                              float width, float height) const;
    glm::mat4 CreateBillboard(const glm::mat4& viewMat, const glm::vec3& centerPosition, const glm::vec3& upDirection,
                              float width, float height) const;

private:
    void InitializeAttackRangeBuffer();
    void InitializeRenderModules();
    void UpdateViewFrustum(const glm::mat4& viewProjectionMatrix) const;
    glm::vec3 ResolveSunDirection(int stageNumber) const;

    void SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat, const glm::vec3& cameraPos,
                     int stageNumber) const;
    glm::mat4 CreateActorModelMatrix(Actor* actor, bool useOrient, float scaleMultiplier = 1.0f) const;
    void DrawActorSelectionUnderlay(Actor* actor, bool useOrient) const;
    void DrawActorOutlineUnderlay(Actor* actor, bool useOrient, const glm::vec4& color, float scaleMultiplier) const;
    void DrawActorSelectionOverlay(Actor* actor, bool useOrient) const;
    void DrawUGCMovingPlatformPaths(const SceneRenderState& scene) const;

    bool UploadActorSkinningMatrices(const Actor* actor) const;
    void SetSkinningEnabled(bool isEnabled) const;
    void DrawActorToShadowMap(Actor* actor, bool useOrient, const glm::vec3& focusPosition) const;

private:
    GLFWwindow& mWindow;
    const MathUtils& mMath;
    MeshLoadSystem& mModelLoader;
    const PhysicsSystem& mPhysics;
    const UILoadSystem& mUIDefinitions;
    CameraSystem& mCameras;
    FramePerformanceTracker& mPerformance;
    std::function<SceneRenderState()> mReadScene;
    std::unique_ptr<Shader3D> mShader3DUnique;
    Shader3D* mShader3D;

    std::unique_ptr<RenderViewportController> mRenderViewportController;
    std::unique_ptr<DirectionalShadowMap> mDirectionalShadowMap;
    std::unique_ptr<EnvironmentDecorationRenderer> mEnvironmentDecorationRenderer;
    std::unique_ptr<SceneObjectRenderer> mSceneObjectRenderer;
    std::unique_ptr<PlayerEffectRenderer> mPlayerEffectRenderer;
    std::unique_ptr<PlanetAtmosphereRenderer> mPlanetAtmosphereRenderer;
    std::unique_ptr<DebugLabelRenderer> mDebugLabelRenderer;
    std::unique_ptr<NPCProximityMessageRenderer> mNPCProximityMessageRenderer;
    std::unique_ptr<ParticleRenderer> mParticleRenderer;

    GLuint mAttackRangeVAO;
    GLuint mAttackRangeVBO;
    mutable std::unordered_set<std::string> mFailedTextureOverrides;
    mutable std::array<glm::vec4, 6> mViewFrustumPlanes{};
    mutable bool mHasValidViewFrustum = false;
    mutable bool mEmphasizeUGCLayers = false;
    mutable bool mDimNonEditingUGCLayers = false;
    mutable int mUGCEditLayer = 0;
    mutable float mActorOpacityMultiplier = 1.0f;
    mutable glm::vec3 mCurrentCameraPosition{0.0f};
    LightingSettings mLightingSettings;
};
