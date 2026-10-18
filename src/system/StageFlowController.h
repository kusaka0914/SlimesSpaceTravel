#pragma once

#include <string>

class ActorLoadSystem;
class AudioSystem;
class PhysicsSystem;
class PauseMenuController;
class SceneSystem;
class GameWorld;
enum class StagePhysicsReloadMode { Rebuild, SkipRebuild };

// ステージYAMLの選択と再読込、物理ワールド・BGMの更新をまとめる。
class StageFlowController {
public:
    StageFlowController();

    // Actor読込成功後、指定モードに応じて物理を再構築しBGMを更新する。読込失敗時はfalse。
    bool ReloadCurrentStage(ActorLoadSystem& actorLoader, PhysicsSystem& physicsSystem, AudioSystem& audioSystem,
                            StagePhysicsReloadMode physicsReloadMode);

    // Stageの選択とYAMLパスを変更する。範囲外では何もしない。データの読込は別途行う。
    void ChangeStage(GameWorld& world, int stageNum);

    // ポーズを閉じ、拠点以外ならフェードを伴う拠点への移動を要求する。
    void ReturnToBase(PauseMenuController& pauseMenu, const GameWorld& world, SceneSystem& sceneSystem);

    void SetCurrentStageYamlPath(const std::string& yamlPath) { mCurrentStageYamlPath = yamlPath; }
    const std::string& GetCurrentStageYamlPath() const { return mCurrentStageYamlPath; }

private:
    std::string mCurrentStageYamlPath;
};
