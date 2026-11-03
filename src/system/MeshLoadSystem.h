#pragma once

#include "system/mesh/AssimpMeshLoader.h"
#include "system/mesh/LoadedModel.h"
#include "system/mesh/MeshCollisionDataLoader.h"
#include "system/mesh/TextureLoader.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Actor;

struct CollisionMeshGeometry {
    std::vector<float> positions;
    std::vector<unsigned int> indices;
};

// 描画モデルと衝突用メッシュを読み込み、パス別にキャッシュする。
class MeshLoadSystem {
public:
    MeshLoadSystem();
    MeshLoadSystem(const MeshLoadSystem&) = delete;
    MeshLoadSystem& operator=(const MeshLoadSystem&) = delete;

    void Initialize();

    // 描画停止後、OpenGLコンテキストが有効な間に呼ぶ。返却済みモデル参照も無効になる。
    // デストラクタではGPU資源を解放しないため、所有するGameが終了順を保証する。
    void Shutdown();

    // Actorのモデルパスを解決し、キャッシュ内のモデルを割り当てる。nullptrでは何もしない。
    void SetActorMesh(Actor* actor);

    // キャッシュ内の衝突用メッシュを返す。空パス・読込失敗時はnullptr。失敗したパスは再読込しない。
    // 同じ場所の「拡張子を.collision.objへ置換したファイル」を優先し、なければ・読めなければ元モデルを使う。
    const CollisionMeshGeometry* ResolveCollisionMeshGeometry(const std::string& modelFilePath);

    // キャッシュを検索する。未登録ならnullptrを返し、読込は行わない。
    const LoadedModel* FindLoadedModel(const std::string& modelPath) const;

    // 未登録なら読み込んでキャッシュする。空パスではnullptr、読込失敗時も空のモデルが登録され得る。
    const LoadedModel* ResolveLoadedModel(const std::string& modelPath);

private:
    LoadedModel LoadModelFromFile(const char* path);
    void CreateLoadedModels();
    void RegisterModel(const std::string& modelPath);

    std::string ResolveModelFilePath(const std::string& modelPath) const;

    TextureLoader mTextureLoader;
    AssimpMeshLoader mAssimpMeshLoader;
    MeshCollisionDataLoader mCollisionDataLoader;

    std::unordered_map<std::string, LoadedModel> mLoadedModels;
    std::unordered_map<std::string, CollisionMeshGeometry> mCollisionMeshGeometryByPath;
    std::unordered_set<std::string> mFailedCollisionMeshPaths;
};
