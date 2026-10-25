#pragma once

#include <yaml-cpp/yaml.h>

struct DebugEditorContext;

// UGC編集が現在のStageへ要求する読込・保存・再構築の境界。Contextを借用し、Actor参照は保持しない。
// ステージ切替後も操作時点の対象を使う。Gameの初期化中・終了後には使用しない。
class UGCStageEditSession {
public:
    explicit UGCStageEditSession(DebugEditorContext& context);

    bool IsAvailable() const;
    bool HasCamera() const;

    // IsAvailable()がtrueのときに呼ぶ。単位はワールド距離。
    float GetGridSize() const;
    void SetPreviewEditLayer(int layer);

    // 失敗時はfalse。LoadDocumentは失敗時に出力を変更せず、保存は旧ファイルを保護する。
    bool LoadDocument(YAML::Node& document) const;
    bool SaveDocument(YAML::Node document) const;
    bool WritePlanetTransforms(YAML::Node& document) const;

    // 成功するとActor参照が無効になる。失敗時の復旧と履歴確定は呼び出し側のTransactionが担当する。
    bool ReloadStage();

private:
    DebugEditorContext& mContext;
};
