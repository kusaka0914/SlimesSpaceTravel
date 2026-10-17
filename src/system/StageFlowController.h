#pragma once

#include <string>

class Game;
class GameWorld;
enum class StagePhysicsReloadMode;

// ステージYAMLの選択と再読込、物理ワールド・BGMの更新をまとめる。
class StageFlowController {
public:
    StageFlowController();

    bool LoadData(Game& game);

    // Actor読込成功後、指定モードに応じて物理を再構築しBGMを更新する。読込失敗時はfalse。
    bool ReloadCurrentStage(Game& game, StagePhysicsReloadMode physicsReloadMode);

    // Stageの選択とYAMLパスを変更する。範囲外では何もしない。データの読込は別途行う。
    void ChangeStage(GameWorld& world, int stageNum);

    // ポーズを閉じ、拠点以外ならフェードを伴う拠点への移動を要求する。
    void ReturnToBase(Game& game);

    void SetCurrentStageYamlPath(const std::string& yamlPath) { mCurrentStageYamlPath = yamlPath; }
    const std::string& GetCurrentStageYamlPath() const { return mCurrentStageYamlPath; }

private:
    std::string mCurrentStageYamlPath;
};
