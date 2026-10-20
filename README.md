# Slime's Space Travel

C++ / OpenGLで制作した、個人制作のステージクリア型3Dアクションゲームです。

球体・楕円体の惑星を360°移動し、惑星の表・側面・裏側を探索しながら、敵との戦闘や2体のスライムを使ったギミックを攻略します。

Unity / Unreal Engineなどの汎用ゲームエンジンは使用せず、描画、物理連携、キャラクター制御、カメラ、アニメーション、敵AI、ステージ管理、開発用エディタなど、ゲームを構成する主要システムをC++で実装しています。

### 制作情報

| 項目 | 内容 |
| --- | --- |
| 制作形態 | 個人制作 |
| 開発期間 | 2026.3〜2026.10（以降も改善継続） |
| 開発時間 | 約654時間（記録範囲での概算） |
| 言語 | C++ / GLSL |
| 開発環境 | Windows |
| 担当範囲 | ゲーム設計 / 全プログラム実装 / ステージ制作 / UI・開発ツール / 一部モデル制作 |

- **プレイ映像:** https://youtu.be/fcE3nDOE_2A
- **実行ファイル:** https://kusaka0914.itch.io/slimesspacetravel
- **開発要点書:** [summary.pdf](summary.pdf)
- **ポートフォリオサイト（他の制作物も紹介しています）:** https://kusaka0914.github.io

<img width="1280" alt="Slime's Space Travel" src="https://github.com/user-attachments/assets/28071dbf-166d-465a-a26b-8b4e9404c04b" />

---

## 制作コンセプト

ゲームに慣れていない人でも迷いにくく、家族や友人と一緒に楽しめる3Dアクションを目指して制作しました。

制作のきっかけは、友人とアクションゲームを遊んだ際に、広いマップや多くのアイコンから次の目的を判断できず、進行に迷っていたことです。そこで本作では、一方向へ進み続けてもいずれ元の場所へ戻れる球体地形を採用し、現在地を見失いにくいステージ構造を目指しました。

一方で探索を単純化しすぎないよう、惑星の表・側面・裏側そのものを探索対象とし、「まだ見ていない場所を探す」楽しさと迷いにくさの両立を図っています。

複数人によるプレイテストに加え、東京ゲームショウ（TGS）での展示も行い、実際のプレイヤーから得られたフィードバックをもとに操作性やステージ導線、ゲーム内容の調整・改善を継続しています。

---

## ゲームの特徴

- 球体・楕円体の表・側面・裏側を移動できる360°ステージ
- 1体のスライムから2体へ分身して攻略するギミック
- コンボ、ガード破壊、打ち上げ、空中攻撃、ため攻撃を組み合わせる戦闘
- 1人プレイ / 2人プレイ対応
- アクションが苦手な人でも遊びやすい「らくらくスタイル」
- 2Dグリッドと3Dプレビューを組み合わせたプレイヤー向けステージ作成機能

<img width="1275" alt="Slime's Space Travel gameplay" src="https://github.com/user-attachments/assets/df8a8d36-4e73-4384-b31f-5915db8f8ca3" />

---

## コードを見る方へ ― まずこの3箇所

短時間で確認いただく場合は、以下の3つをご覧ください。

1. **360°移動** — 3D数学・物理・キャラクター制御
2. **プレイヤー向けStage Editor** — 2D入力と3D空間の対応・編集システム
3. **Build & Restart** — C++開発の確認サイクルを短縮する開発ツール

### 1. 球体・楕円体上の360°移動

#### まず見るコード

[Planet.cpp](src/actor/Planet.cpp)

`Planet::CalculateEllipseSurfaceProjection`

#### 課題

本作では、ワールド座標のY軸を常に上方向として扱わず、Actorごとに変化する `upVec` を基準に接地・姿勢・移動・重力・カメラを制御しています。

球体では「惑星中心からActorへの方向」を表面法線として利用できますが、楕円体では中心方向と実際の表面法線が一致しません。

#### 実装

`CalculateEllipseSurfaceProjection` では、ワールド座標上の点に対して楕円体表面上の対応位置を求め、そこから外向き法線を計算しています。

楕円体外部の点については、制約式を満たすパラメータを二分探索で求め、表面上の最近点を算出します。

算出した表面位置・法線・表面までの距離は、PlayerやEnemyの姿勢、重力方向、惑星との距離判定などから共通利用しています。

**計算上の制約：** 対象はワールド軸に沿う楕円体です。外部点は最近傍、内部点は中心からの放射方向へ投影します。外部点の二分探索は48回で打ち切ります。内部点でも最近点を求める方式とは結果が異なるため、両者を契約として分けています。

テストでは、楕円体の表面方程式、法線の向き、中心点・内部点・外部点、軸の入替、細長い楕円体を確認します。反復回数と精度の関係は、すべての浮動小数点入力に対する誤差保証ではありません。

#### 関連コード

- [ActorGroundResolver.cpp](src/actor/ActorGroundResolver.cpp)
  - `CalculateAverageNormal`
  - `CalculateFallbackUpVec`
- [PlayerPlanetGravityController.cpp](src/actor/player/PlayerPlanetGravityController.cpp)
  - `CalculateAirbornePhysicsUpDirection`
  - `Update`
- [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)
- [CameraCollisionResolver.cpp](src/system/camera/CameraCollisionResolver.cpp)

接地中はActorの中央・前後左右からRay Testを行い、複数の地面法線から安定した上方向を求めます。

空中では周囲の惑星との位置関係から重力対象を切り替え、姿勢が急激に変化しないよう `upVec` を補間します。

カメラもPlayerの `upVec` を基準に姿勢を構築し、惑星の裏側へ移動しても操作方向を維持できるようにしています。

### 2. 2D操作で3Dステージを作るStage Editor

#### まず見るコード

[UGCEditorInteractionController.cpp](src/editor/ugc/UGCEditorInteractionController.cpp)

`UGCEditorInteractionController::UpdateSelectionDrag`

#### 課題

一般的な3DエディタのようにX・Y・Z軸を直接操作する方式では、ゲームを遊ぶプレイヤーがステージを作成するには操作が複雑になります。

そこで、

- 水平方向：2Dグリッド
- 高さ方向：「だん」

として分離し、「1マス動かす」「1だん上げる」といった操作で3Dステージを作れる方式にしました。

#### 実装

マウス位置から3D空間へRayを生成し、編集用Planeとの交点を求めます。

交点から移動量を計算し、グリッドサイズへスナップしてActorを移動します。

ドラッグ開始時の位置や適用済みの移動量を保持し、Undo / RedoやYAML上の保存データとの整合性も管理しています。

**保存と履歴：** ドラッグ中はRuntime上でプレビューし、確定時に保存します。履歴はYAML全体のスナップショットなので、編集差分ごとの復元処理を持たずに戻せる一方、大きなステージでは履歴のメモリ量と再読込コストが増えます。保存・再読込が失敗した場合は以前の状態への復旧を試み、成功してから履歴を進めます。

#### 関連コード

- [UGCPreviewController.cpp](src/gameplay/ugc/UGCPreviewController.cpp)
- [GameFrameRenderer.cpp](src/app/GameFrameRenderer.cpp)
  - `DrawUGCPreviewFrame`

編集内容はゲーム本体とは別のFramebufferへ描画し、2D編集画面と3Dプレビューを切り替えながら確認できます。

### 3. 編集状態を保持したBuild & Restart

#### まず見るコード

[EditorBuildRestartService.cpp](src/editor/session/EditorBuildRestartService.cpp)

#### 課題

本作ではC++コードを変更して動作確認するたびに、

`Build → ゲーム起動 → Editorを開く → 編集中の場所まで戻る`

という操作が必要でした。

機能が増えるにつれて確認までの操作が増えたため、この反復を短縮する仕組みを実装しました。

#### 実装

Editor上でBuild & Restartを実行すると、

1. 現在のEditor状態を保存
2. Helper Processを起動
3. ゲーム本体を終了
4. Helper Process側でBuild
5. Build成功後にゲームを再起動
6. 保存していたEditor状態を復元

という流れで処理します。

Buildをゲーム本体とは別Processへ任せることで、自身の実行ファイルを終了した後でもBuildと再起動を継続できる構成にしています。

**方式の制約：** 実行中コードの差し替えではなく、プロセスを再起動します。復元するのは保存したEditor状態であり、実行中のすべてのオブジェクトのメモリ状態ではありません。Helperの引数検査と、起動失敗・ビルド失敗の経路もテスト対象です。

Windowsでは `CreateProcessW`、POSIX環境では `fork` / `execv` を利用しています。POSIXでは起動結果をパイプで親へ通知し、作業ディレクトリ変更や実行ファイルの起動に失敗した場合もエラーを表示します。

#### 関連コード

- [DebugEditorSessionController.cpp](src/editor/session/DebugEditorSessionController.cpp)
- `tools/editor_restart_helper/`

---

## 設計について

機能追加を続けても処理の流れと変更箇所を追いやすくするため、プロジェクト全体で**変更理由の異なる処理を一つのクラスへ集めすぎないこと**を意識しています。

例えばPlayerでは、

- 入力
- 移動
- 接地
- 惑星重力
- 戦闘
- 状態管理

をそれぞれ専用クラスへ分離し、Player本体は外部から利用する窓口として各処理へ委譲しています。

Enemyについても、移動・戦闘・ダメージ処理・状態・行動AIを分離しています。

ゲーム全体についても、

- Camera
- Physics
- Mesh / Texture読み込み
- Scene
- UI
- Stage読み込み

などをそれぞれ役割ごとのSystemへ分けています。

また、

- 複数Actorで利用する処理はComponentとして共通化
- 調整頻度の高い値はYAMLへ分離
- 複雑な条件は意味を表す変数や判定関数へ分離
- 所有権が必要な箇所ではスマートポインタを利用
- `const` / `constexpr` で変更可能性を明確化

することを意識しています。

### 自動テスト

画面を必要としないデータ処理・状態管理と、OpenGLを使うRuntime統合テストを分けています。保存失敗時の復旧、Undo / Redo、入力機器の割当、戦闘状態、惑星の幾何条件などを検証します。[実行手順](#品質検証の入口)を参照してください。

---

## その他の実装

<details>
<summary><strong>YAMLで行動を組み替えられるAction型Enemy AI</strong></summary>

### 主なコード

- [EnemyBehaviorController.cpp](src/actor/enemy/behavior/EnemyBehaviorController.cpp)
- [EnemyBehaviorAction.h](src/actor/enemy/behavior/EnemyBehaviorAction.h)
- [EnemyBehaviorActionFactory.cpp](src/actor/enemy/behavior/EnemyBehaviorActionFactory.cpp)
- [EnemyConfigLoader.cpp](src/actor/enemy/EnemyConfigLoader.cpp)

Enemyの種類ごとに大きな条件分岐を書くのではなく、「待機」「追跡」「近接攻撃」「扇形攻撃」「突進」などを独立したActionとして実装しています。

各Actionは `CanStart` / `CanContinue` / `Evaluate` を持ち、実行可能なActionから評価値を利用して次の行動を選択します。

使用するActionとパラメータはYAMLから変更できます。

</details>

<details>
<summary><strong>OpenGL Timer QueryによるGPU処理時間計測</strong></summary>

### 主なコード

- [GpuDurationTimer.cpp](src/gfx/performance/GpuDurationTimer.cpp)
- [GameFrameRenderer.cpp](src/app/GameFrameRenderer.cpp)

`GL_TIME_ELAPSED` を利用してGPU実行時間を測定しています。

結果取得時にCPUを待たせないよう複数のQuery Slotを循環利用し、`GL_QUERY_RESULT_AVAILABLE` で完了したQueryのみ後から取得します。

</details>

<details>
<summary><strong>ゲーム画面上で直接編集できるUI Editor</strong></summary>

### 主なコード

- [UICanvasEditorController.cpp](src/editor/ui/UICanvasEditorController.cpp)
- [UILoadSystem.cpp](src/system/UILoadSystem.cpp)

実際のゲーム画面上でUIを選択し、

- 移動
- 回転
- 拡大縮小
- 複製
- 削除
- Undo
- YAML保存

まで行えます。

複数選択、範囲選択、重なったUIの選択切り替えにも対応しています。

</details>

<details>
<summary><strong>Sweep判定による壁衝突・スライド移動</strong></summary>

### 主なコード

- [ActorCollisionResolver.cpp](src/system/physics/ActorCollisionResolver.cpp)

移動開始位置から終了位置までSweep判定を行い、高速移動時のすり抜けを抑えています。

壁へ衝突した場合は、残りの移動量から衝突法線方向へ入り込む成分を除き、壁面に沿って移動させます。

</details>

<details>
<summary><strong>分身・1人 / 2人プレイを共通化したPlayer管理</strong></summary>

### 主なコード

- [PlayerConfigurationController.cpp](src/system/PlayerConfigurationController.cpp)

1人プレイ時の分身と2人プレイ時のPlayer参加を共通のPlayer構造で管理しています。

分身位置も固定ワールド座標ではなく、現在の `upVec` を利用して惑星表面に沿うよう計算しています。

</details>

<details>
<summary><strong>YAMLベースのSequence System</strong></summary>

### 主なコード

- [SequenceSystem.cpp](src/system/sequence/SequenceSystem.cpp)
- [SequenceTypes.h](src/system/sequence/SequenceTypes.h)
- [SequenceLibrary.cpp](src/system/sequence/SequenceLibrary.cpp)

Actor移動、表示切り替え、Player操作、Camera演出などを時間軸上のClipとして扱います。

SequenceをYAMLから読み書きすることで、ゲームロジックへ個別の演出処理を書き込まずに構成できます。

</details>

<details>
<summary><strong>日本語文章から自動生成するルビ表示</strong></summary>

### 主なコード

- [JapaneseRubyGenerator.cpp](src/system/text/JapaneseRubyGenerator.cpp)
- [RubyText.h](src/text/RubyText.h)
- [UICustomElementRenderer.cpp](src/gfx/ui/UICustomElementRenderer.cpp)

Windows版では `Windows::Globalization::JapanesePhoneticAnalyzer` を利用して日本語文章から読みを取得し、漢字を含む部分へ自動でルビを表示します。

解析結果から本文を再構築し、元の文章と一致することを確認した場合のみ利用しています。

</details>

<details>
<summary><strong>ボーンアニメーションの再生・補間</strong></summary>

### 主なコード

- [AnimationPlayer.cpp](src/animation/AnimationPlayer.cpp)

Assimpから読み込んだKeyframeを利用し、

- Position / Scale：線形補間
- Rotation：Quaternion SLERP

で補間します。

Skeleton階層を再帰的に辿り、最終的なBone Transformを計算しています。

</details>

---

## 使用技術

| 分野 | 使用技術 |
| --- | --- |
| 言語 | C++ / GLSL |
| Graphics | OpenGL / GLEW |
| Window / Input | GLFW / SDL2 |
| Physics | Bullet Physics |
| 3D Model / Animation | Assimp |
| Audio | SDL_mixer |
| Font | SDL_ttf |
| Data | YAML / yaml-cpp |
| Math | GLM |
| Development UI | Dear ImGui / ImGuizmo |
| Build / Test | CMake / CTest / vcpkg |

Unity / Unreal Engineなどの汎用ゲームエンジンは使用していません。

<details>
<summary><strong>主な技術選定理由</strong></summary>

- **C++ / OpenGL**  
  汎用ゲームエンジンが提供する機能を利用するだけでなく、描画、キャラクター制御、カメラ、物理連携など、3Dゲームを構成する仕組みを自分で実装しながら理解するために採用しました。

- **Bullet Physics**  
  球体・楕円体上の接地や、惑星上に配置した足場との衝突などに必要なRay Test / Sweep Test / Contact Testを利用するために採用しました。判定結果を利用し、接地・壁衝突・カメラ衝突など本作独自の処理を実装しています。

- **YAML / yaml-cpp**  
  ステージ、UI、Enemy AI、Camera、Sequenceなど、調整頻度の高いデータをソースコードから分離するために採用しました。階層構造を扱いやすく、調整中にコメントを記述できる点も理由の一つです。

- **CMake / vcpkg**  
  ビルド構成と外部ライブラリの依存関係を管理し、開発環境を再構築しやすくするために採用しました。

</details>

---

## コード構成

```text
src/
├─ app/               Game・Window・入力/Sessionの調停とSystemの組み立て
├─ world/             Stage・Actorの所有と寿命管理
├─ actor/
│  ├─ player/         Playerの入力・移動・重力・戦闘・状態
│  ├─ enemy/          Enemyの移動・戦闘・状態・AI
│  └─ planet/         惑星上のActor・進行管理
│
├─ animation/         Skeletal Animation
├─ component/         Actor間で再利用する機能
├─ effect/            Particleなどの演出
├─ gameplay/ugc/      UGCの実行状態・作品保存・プレビュー状態
│
├─ editor/
│  ├─ core/           Editor全体の調停・Context・Viewport
│  ├─ stage/          Stageの配置・Inspector・Undo / Redo・YAML保存
│  │  ├─ creation/       Actorの作成・配置
│  │  ├─ selection/      選択・Pick・Gizmo
│  │  ├─ inspectors/     Actorの設定編集UI
│  │  ├─ persistence/    YAML読込・保存・足場参照
│  │  ├─ history/        編集操作・Undo / Redo・Transaction
│  │  ├─ ugc/            UGCのセル・グリッド編集
│  │  └─ panels/         Stage Editorのパネル
│  ├─ ugc/            UGC編集の入力・表示・操作
│  ├─ ui/             UI Canvasの編集・選択・履歴
│  ├─ panels/         基本情報・性能表示
│  ├─ tools/          Camera・Particle・Sequenceなどの編集
│  ├─ session/        編集状態の保存・復元・Build & Restart
│  └─ assets/         Editor用Asset Browser・サムネイル
│
├─ gfx/
│  ├─ render3d/       3D描画
│  ├─ ui/             HUD・Menu・Tutorial
│  ├─ particle/       Particle描画
│  ├─ performance/    GPU計測
│  └─ video/          動画表示
│
├─ system/
│  ├─ actor_loader/   YAMLからのStage構築
│  ├─ camera/         Camera制御
│  ├─ mesh/           Model / Texture / Collision読み込み
│  ├─ physics/        Bullet Physics・移動衝突判定
│  └─ scene/          Scene・会話・Tutorial
│
├─ state/             ゲーム進行・UI状態
├─ text/              日本語・Ruby表示用データ
└─ utils/             AtomicFile・YAML読み取り・文字列処理
```

`app`がSystemとEditorを組み立て、EditorはRuntimeのAPIを利用します。`gfx`とUGCのRuntimeはEditor内部へ依存しません。Actor専用の姿勢計算は`actor/MathUtils`に置き、`utils`はゲーム固有型へ依存させていません。

### 所有権と変更の境界

- `Game`はSystemとEditorを組み立て、`GameWorld`がActorとStageを所有します。StageやPlanetのActor一覧は借用参照です。
- Stage再読込の前にSequence・Camera・Scene・Editorの旧Actor参照を解除します。読込失敗時は以前のWorldを保持します。描画用の`SceneRenderState`は呼出中だけ有効な一覧を借用し、次の更新や再読込へ持ち越しません。
- Editorの作成・削除・Undo / Redoは`StageEditTransaction`で保存とRuntime再構築を調停します。Inspectorの即時編集は画面に残し、保存失敗の理由と再試行を`StageActorSaveState`で管理します。履歴への追加は保存成功後です。
- `StageProgressSystem`の更新は`ProgressUpdateResult`を返し、メモリ上の変更と保存の成否を区別します。保存に失敗しても現在のゲーム進行を巻き戻しません。
- MeshのGPUリソースはMesh管理側、UIテクスチャはRenderer、Particle用テクスチャはParticleRendererが解放します。アップロード処理は`TextureUploadState`で呼出前のGL状態へ戻します。生成・使用・解放には有効なGLコンテキストが必要です。

### 品質検証の入口

回帰テストは[tests/](tests/)にあり、[unit/](tests/unit/)は個別機能、[integration/](tests/integration/)はゲーム本体や外部プロセスとの連携、[support/](tests/support/)は共通の検証関数と補助プログラムです。静的解析・依存境界・アセット検証の実行スクリプトは[tools/](tools/)にあります。以下の手順で検証できます。

代表的な回帰テストは、ビルボードの退化軸、SDL仮想機器の同数交換、画像読込順とGL状態、再起動のPIDとexec失敗、進行保存の失敗通知、初期化失敗の終了コードです。Player演出へ渡したParticleSystemだけが更新されることと、足場の安定IDだけによる参照も確認します。

```sh
# Debug・警告・表示環境を使う統合テスト
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGAME_ENABLE_WARNINGS=ON -DGAME_BUILD_RUNTIME_TESTS=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure

# 入出力・UGCを含む重要な実装の解析と、既存の依存境界の検査
python3 -B tools/run_static_analysis.py --build build
# 全自作実装を調べる場合は --all-production を付ける。
python3 -B tools/architecture_audit.py --check --output build/audit
python3 -B tools/check_format.py --base <比較元コミット> --clang-format <clang-format-18>

# 既存セーブを使わず、表示したウィンドウで本体のRunLoopを操作する（統合テスト有効時）。
python3 -B tools/run_runtime_checks.py --build build -- --interactive

# Clangでの関数・行・分岐カバレッジ
cmake -S . -B build -DGAME_ENABLE_COVERAGE=ON
cmake --build build --parallel 2
python3 -B tools/collect_coverage.py --build build --run-tests

# 計装を外したReleaseで固定入力のCPU計測
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DGAME_ENABLE_COVERAGE=OFF -DGAME_BUILD_BENCHMARKS=ON
cmake --build build --parallel 2
python3 -B tools/run_benchmarks.py --build build

# Releaseの実描画ループを1人・2人・UGCで各600フレーム計測する。
# Windowsではpython、複数構成ジェネレーターでは --configuration Release を使う。
python3 -B tools/measure_frame_loop.py --build build
# 繰り返し遷移の長時間確認（60FPSなら約10分）。既存セーブは使用しない。
python3 -B tools/measure_frame_loop.py --build build --frames 36000 --scenario cycle --output build/benchmarks/soak.json
```

検証の適用範囲は次のとおりです。

- 静的解析は選択した翻訳単位、依存検査は引用形式includeを対象とし、全経路の安全性は証明しません。
- カバレッジには未実行の自作実装も含めます。プロファイル不一致の診断がある場合、割合だけで評価しません。
- CPUベンチマークはメモリ内のYAML処理・Actor生成破棄・姿勢計算を対象とします。実描画のフレーム時間は`measure_frame_loop.py`で別に記録します。異なる測定条件の自動比較は拒否します。
- 同梱YAMLを検査し、重複キー・非有限ベクトル・惑星参照・足場ID・Pressure Switch接続の不整合を検出します。
- [CI](.github/workflows/quality.yml)はLinuxのDebug / Release・統合テスト・解析と、WindowsのReleaseビルド・画面不要のテストを対象とします。結果はActionsで確認してください。
- Windows実機の操作・表示・音声・性能は、CIの成功とは別に確認が必要です。測定結果は記録された機器・構成にだけ適用されます。

<details>
<summary>macOSでRuntimeへSanitizerを適用する場合</summary>

`GAME_ENABLE_TEST_SANITIZERS=ON`は画面不要の単体テスト、`GAME_ENABLE_RUNTIME_SANITIZERS=ON`はゲーム本体とRuntime統合テストへAddressSanitizer / UBSanを適用します。両方を有効にすると双方を検証できます。

macOSのlibc++では、計装済みコードと未計装ライブラリの混在でコンテナ検査が誤報することがあります（[Clangの制約](https://clang.llvm.org/docs/AddressSanitizer.html#container-overflow-detection)）。同じコンパイラーとSanitizerでyaml-cppもビルドし、`-Dyaml-cpp_DIR=<インストール先>/lib/cmake/yaml-cpp`を指定してください。通常構成へ戻す際はこの指定も戻します。共有ライブラリ内部の計装や、利用環境で非対応のリーク検査は保証しません。

</details>

---

## 実行方法

実行ファイル一式はitch.ioからダウンロードできます。

https://kusaka0914.itch.io/slimesspacetravel

zipを展開し、実行ファイルを起動してください。

---

## Windowsでのビルド

### 必要環境

- Windows 10 / 11 64bit
- Visual Studio 2022
  - 「C++によるデスクトップ開発」
  - 「Windows用C++ CMakeツール」（CMake 3.18以降）
- Git
- インターネット接続（初回ビルド時）

### ビルド方法

`build_windows.bat` を実行してください。

初回実行時は必要に応じてvcpkgを自動取得し、`vcpkg.json`に記載された依存ライブラリを取得してWindows x64 Release版をビルドします。新規取得するvcpkg本体と、manifestの`builtin-baseline`はCIと同じコミットを使用します。`VCPKG_ROOT`を指定した場合は既存のvcpkg本体を利用します。

CMakeはPATH、次にVisual Studio同梱版から探します。`build_windows.bat --check-cmake`で、依存ライブラリを取得せず探索だけを確認できます。

`game`をビルドすると`editor_restart_helper`も生成され、`--debug`で起動したEditorからBuild & Restartを利用できます。

ビルド後は以下を実行できます。

`out/build/windows-x64-release/Release/game.exe`

必要な `assets` / `shaders` も自動配置されます。

---

## 開発用Debug Editor

実行時に `--debug` を指定すると開発用Editorを利用できます。

```bash
<実行ファイル名> --debug
```

Debug Modeでは、

- 3D Stage Editor
- UI Editor
- 自由Camera
- Stage / UI Data再読み込み
- Parameter調整
- Asset確認
- Performance計測

などを利用できます。

---

<details>
<summary><strong>生成AIの利用について</strong></summary>

本作では、生成AIを補助的に利用しています。

### 画像アセット

生成AIで作成した画像を以下の一部で使用しています。

- UI用画像
- オープニング演出用画像
- 一部の2Dテクスチャ素材

3Dモデル・アニメーションは生成AIで生成したものではありません。

### プログラム開発

設計案の検討、問題の調査、コードレビューに加え、コード案の作成、実装・リファクタリング、テスト作成の補助にも生成AIを利用しています。

生成AIによる提案にも誤りがあるため、採用したコードは実装の確認とテストの対象としています。AI利用を含む担当範囲の説明と、提出するコードの最終責任は制作者が持ちます。自動検証の範囲と、実機確認が必要な範囲は[品質検証の入口](#品質検証の入口)に記載しています。

</details>

---

## 詳細資料

制作背景・コンセプト、開発期間、制作人数、担当範囲、ゲームデザイン、技術選定理由、開発用Editor、システム設計、技術的に工夫した点などは、以下にまとめています。

[開発要点書（summary.pdf）](summary.pdf)
