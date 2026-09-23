# Slime's Space Travel

C++20 / OpenGLで制作した、ステージクリア型3Dアクションゲームです。

球体や楕円状の惑星を360°移動し、惑星の表・側面・裏側を探索しながら、
敵との戦闘や2体のスライムを使ったギミックを攻略します。

Unity / Unreal Engineなどの汎用ゲームエンジンは使用せず、
描画、物理連携、キャラクター制御、カメラ、アニメーション、
ステージ管理、開発用エディタなど、ゲームを構成する主要システムを実装しています。

- **プレイ映像:** https://youtu.be/GYYkx0_Y8g8?si=9aH_WY7Vdzd2oqSv
- **実行ファイル:** https://github.com/kusaka0914/SlimesSpaceTravel/releases
- **開発要点書:** [summary.pdf](summary.pdf)
- **ポートフォリオサイト（他の制作物についても掲載しています。）:** https://kusaka0914.github.io

<img width="1280" alt="Slime's Space Travel" src="https://github.com/user-attachments/assets/28071dbf-166d-465a-a26b-8b4e9404c04b" />

---

## コードを見る方へ ― まずこの3箇所

本プロジェクトは機能数・ファイル数が多いため、
本作らしい技術・ゲーム性・設計上の工夫が分かりやすい箇所を3点に絞っています。

短時間で確認いただく場合は、以下の順番でご覧ください。

### 1. 球体・楕円体上の360°移動・重力・カメラ

**まず見るファイル**

- [Planet.cpp](src/actor/Planet.cpp)
  - `CalculateEllipseSurfaceProjection`
- [ActorGroundResolver.cpp](src/actor/ActorGroundResolver.cpp)
  - `CalculateAverageNormal`
  - `CalculateFallbackUpVec`
- [PlayerPlanetGravityController.cpp](src/actor/player/PlayerPlanetGravityController.cpp)
  - `Update`
  - `CalculateAirbornePhysicsUpDirection`
  - `OnJumpStarted`
- [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)
  - `UpdateState`
  - `GetView`
- [CameraCollisionResolver.cpp](src/system/camera/CameraCollisionResolver.cpp)
  - `Resolve`

本作では、ワールド座標のY軸を常に上方向として扱うのではなく、
Actorごとに変化する `upVec` を基準として、接地・姿勢・移動・重力・カメラを制御しています。

接地中は、Actorの中央・前後左右から地面方向へRay Testを行い、
取得した地面法線から上方向を決定します。

楕円体では「惑星中心からActorへの方向」と表面法線が一致しないため、
楕円体表面上の最近点を求め、その位置から外向き法線を計算しています。
楕円体外部の点については、制約式を満たすパラメータを二分探索で求めています。

空中では周囲の惑星との位置関係から重力対象を切り替え、
急激に姿勢が変化しないよう `upVec` を補間します。

カメラも固定Y軸ではなくプレイヤーの `upVec` を基準に姿勢を構築し、
惑星表面を移動して上方向が変化しても操作方向を維持します。
地形へのめり込みはBullet PhysicsのRay Test / Contact Testで補正しています。

**見るポイント**

- 楕円体表面上の最近点・法線を独自に計算している点
- 複数のRay Testから安定した地面法線を求めている点
- 空中で重力対象を切り替えながら姿勢を補間している点
- 任意の上方向を基準にカメラを制御している点
- Ray Test / Contact Testでカメラの衝突を処理している点

---

### 2. 2D操作で3Dステージを作るプレイヤー向けステージエディタ

**まず見るファイル**

- [UGCEditorInteractionController.cpp](src/gfx/debug/ugc/UGCEditorInteractionController.cpp)
  - `MoveSelectionOnGrid`
  - `UpdateSelectionDrag`
  - `ChangeLayer`
  - `HandleUndo`
  - `HandleRedo`
- [UGCPreviewController.cpp](src/gfx/debug/ugc/UGCPreviewController.cpp)
  - `SetEditLayer`
  - `AdjustYaw`
  - `UpdateFocusY`
- [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)
  - `DrawUGCPreviewFrame`

プレイヤー自身がオリジナルステージを作成できる機能です。

一般的な3DエディタのようにX・Y・Z軸を自由に操作する方式ではなく、
水平方向を2Dグリッド、高さ方向を「だん」として分離しています。

内部では3D座標を扱いますが、
プレイヤーは「1マス移動する」「1だん上げる」といった操作で
立体的なステージを制作できます。

マウス操作は3D空間上のドラッグへ変換し、
配置位置はグリッドサイズへスナップします。

編集内容はゲーム本体とは別のFramebufferへ3D描画し、
2D編集画面と3Dプレビューを行き来しながら確認できます。

**見るポイント**

- 3D空間を2Dグリッド＋高さレイヤーへ置き換えている点
- マウス入力を3D空間上の移動へ変換している点
- 配置位置をグリッド単位へスナップしている点
- Undo / Redoを含む編集操作を管理している点
- 専用Framebufferへ3Dプレビューを描画している点

---

### 3. YAMLで行動を組み替えられるAction型Enemy AI

**まず見るファイル**

- [EnemyBehaviorController.cpp](src/actor/enemy/behavior/EnemyBehaviorController.cpp)
  - `Configure`
  - `Update`
  - `SelectAction`
  - `SwitchAction`
- [EnemyBehaviorActionFactory.cpp](src/actor/enemy/behavior/EnemyBehaviorActionFactory.cpp)
  - `Create`
- [EnemyBehaviorAction.h](src/actor/enemy/behavior/EnemyBehaviorAction.h)
- [EnemyConfigLoader.cpp](src/actor/enemy/EnemyConfigLoader.cpp)
  - `Parse`

敵の種類ごとに大きな条件分岐を書くのではなく、
「待機」「追跡」「近接攻撃」「扇形攻撃」「突進」などの行動を
独立したActionとして実装しています。

各Enemyが使用するActionとAction固有のパラメータはYAMLから読み込みます。

実行時には `CanStart` を満たすActionだけを候補とし、
各Actionが返す評価値と設定された重みを利用して次の行動を選択します。

具体的な各Actionは `src/actor/enemy/behavior/actions/` に配置しています。

**見るポイント**

- Enemyの行動を `EnemyBehaviorAction` として分離している点
- FactoryとYAMLを組み合わせて行動構成をデータ化している点
- `CanStart` / `CanContinue` によって実行条件をAction側へ分離している点
- 実行可能なActionから評価値を利用して次の行動を選択している点
- 新しいAction追加時にEnemy本体の変更範囲を抑えている点

---

## ゲームの主な特徴

- 球体・楕円体の表・側面・裏側を移動できる360°ステージ
- 1体のスライムから2体へ分身して攻略するギミック
- コンボ、ガード破壊、打ち上げ、空中攻撃、ため攻撃を組み合わせる戦闘
- 1人プレイ / 2人プレイ対応
- アクションが苦手な人でも遊びやすい「らくらくスタイル」
- 2Dグリッドと3Dプレビューを組み合わせたステージ作成機能

<img width="1275" alt="Slime's Space Travel gameplay" src="https://github.com/user-attachments/assets/df8a8d36-4e73-4384-b31f-5915db8f8ca3" />

---

## 使用技術

| 分野 | 使用技術 |
| --- | --- |
| 言語 | C++20 / GLSL |
| Graphics | OpenGL / GLEW |
| Window / Input | GLFW / SDL2 |
| Physics | Bullet Physics |
| 3D Model / Animation | Assimp |
| Audio | SDL_mixer |
| Font | SDL_ttf |
| Data | YAML / yaml-cpp |
| Math | GLM |
| Development UI | Dear ImGui / ImGuizmo |
| Build | CMake / vcpkg |

Unity / Unreal Engineなどの汎用ゲームエンジンは使用していません。

---

## コード構成

主要な責務は、以下のようにディレクトリ・クラスへ分離しています。

```text
src/
├─ actor/
│  ├─ player/         Playerの入力・移動・重力・戦闘・状態・Animation
│  ├─ enemy/          Enemyの移動・戦闘・状態・AI
│  └─ planet/         惑星上のActor・進行管理
│
├─ animation/         Skeletal Animationの再生・補間
├─ component/         Actorへ付与する再利用可能な機能
├─ effect/            Particleなどの演出
│
├─ gfx/
│  ├─ debug/
│  │  ├─ stage/       開発用3D Stage Editor
│  │  └─ ugc/         プレイヤー向けStage Editor
│  ├─ render3d/       3D描画
│  └─ ui/             HUD・Menu・Tutorial
│
├─ system/
│  ├─ actor_loader/   YAMLからのStage構築
│  ├─ camera/         Camera制御
│  ├─ mesh/           Model / Texture / Collision読み込み
│  ├─ physics/        Bullet Physics・移動衝突判定
│  └─ scene/          Scene・会話・Tutorial
│
├─ text/              日本語・Ruby表示用データ
└─ Game.cpp           各System / Controllerを保持する全体の調停役
```

---

## 設計で意識したこと

機能追加を続けても変更箇所を追いやすいよう、
役割の異なる処理を一つのクラスへ集めすぎないことを意識しています。

Playerは、入力・移動・接地・惑星重力・戦闘・状態管理などを専用クラスへ分離し、
Player本体は外部から利用する窓口として各処理を委譲しています。

Enemyも、移動・戦闘・ダメージ処理・状態管理・行動AIなどを分離しています。

また、以下を意識しています。

- 複数Actorで再利用する機能はComponentへ分離
- 調整頻度の高い値はYAMLへ分離
- 関数名・クラス名から処理の目的が分かるようにする
- 複雑な条件は意味の分かる変数や判定関数へ分離
- `const` / `constexpr` を利用して変更可能性を明確化
- 開発用Editorを用意し、調整と確認の反復を短縮

---

## さらに見る場合

以下は主導線から外していますが、本作で実装した機能です。

<details>
<summary><strong>OpenGL Timer QueryによるGPU Performance計測</strong></summary>

### 主なファイル

- [GpuDurationTimer.cpp](src/gfx/performance/GpuDurationTimer.cpp)
- [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)
  - `PollGpuPerformanceMeasurements`

`GL_TIME_ELAPSED` を利用してGPU実行時間を測定しています。

計測結果をすぐ取得してCPUを待たせないよう複数のQuery Slotを循環利用し、
`GL_QUERY_RESULT_AVAILABLE` で完了済みのQueryだけを後から取得しています。

</details>

<details>
<summary><strong>編集状態を保持したBuild & Restart</strong></summary>

### 主なファイル

- [EditorBuildRestartService.cpp](src/system/EditorBuildRestartService.cpp)
- [DebugEditorSessionController.cpp](src/system/DebugEditorSessionController.cpp)

C++コード変更後の確認を短縮するため、
現在のEditor状態を保存してから別のHelper ProcessでBuild・再起動し、
起動後に同じ編集状態を復元します。

Windowsでは `CreateProcessW`、
その他の環境では `fork` / `execl` を利用しています。

</details>

<details>
<summary><strong>ゲーム画面上で直接編集できるUI Editor</strong></summary>

### 主なファイル

- [UICanvasEditorController.cpp](src/gfx/debug/ui/UICanvasEditorController.cpp)
- [UILoadSystem.cpp](src/system/UILoadSystem.cpp)

実際のゲーム画面上でUIを選択し、
移動・回転・拡大縮小・複製・削除・Undo・YAML保存まで行えます。

複数選択、範囲選択、重なったUIの選択切り替えにも対応しています。

</details>

<details>
<summary><strong>Sweep判定を使った壁衝突・スライド移動</strong></summary>

### 主なファイル

- [ActorCollisionResolver.cpp](src/system/physics/ActorCollisionResolver.cpp)

移動開始位置から終了位置までSweep判定を行い、
高速移動時のすり抜けを抑えています。

壁へ衝突した場合は、
残りの移動量から衝突法線方向へ入り込む成分を除去し、
壁面に沿って移動させています。

</details>

<details>
<summary><strong>分身・1人 / 2人プレイを共通化したPlayer管理</strong></summary>

### 主なファイル

- [PlayerConfigurationController.cpp](src/system/PlayerConfigurationController.cpp)

1人プレイ時の分身と、
2人プレイ時のPlayer参加を共通のPlayer構造で管理しています。

分身位置もワールド座標固定ではなく、
現在の `upVec` を使って惑星表面に沿うよう計算しています。

</details>

<details>
<summary><strong>YAMLベースのSequence System</strong></summary>

### 主なファイル

- [SequenceSystem.cpp](src/system/sequence/SequenceSystem.cpp)
- [SequenceTypes.h](src/system/sequence/SequenceTypes.h)
- [SequenceLibrary.cpp](src/system/sequence/SequenceLibrary.cpp)

Actor移動、表示切り替え、Player操作、Camera演出などを
時間軸上のClipとして扱います。

SequenceはYAMLから読み書きでき、
ゲームロジックへ個別に演出処理を書き込まずに構成できます。

</details>

<details>
<summary><strong>日本語文章から自動生成するルビ表示</strong></summary>

### 主なファイル

- [JapaneseRubyGenerator.cpp](src/system/text/JapaneseRubyGenerator.cpp)
- [RubyText.h](src/text/RubyText.h)
- [UICustomElementRenderer.cpp](src/gfx/ui/UICustomElementRenderer.cpp)

Windows版では `Windows::Globalization::JapanesePhoneticAnalyzer` を利用して
日本語文章から読みを取得し、漢字を含む部分へ自動でルビを表示します。

解析結果から本文を再構築し、元の文章と一致することを確認してから利用しています。

</details>

<details>
<summary><strong>ボーンアニメーションの再生・補間</strong></summary>

### 主なファイル

- [AnimationPlayer.cpp](src/animation/AnimationPlayer.cpp)

Assimpから読み込んだKeyframeを使用し、
Position / Scaleは線形補間、
RotationはQuaternion SLERPで補間します。

Skeleton階層を再帰的に辿り、最終的なBone Transformを計算しています。

</details>

---

## 実行方法

実行ファイル一式はGitHub Releasesからダウンロードできます。

https://github.com/kusaka0914/SlimesSpaceTravel/releases

Releasesには、実行ファイル、必要なライブラリ、assets、shadersを含む
配布用ファイルを公開しています。

zipを展開し、実行ファイルを起動してください。

---

## ソースからのビルド

ビルドにはCMake、
依存ライブラリ管理にはvcpkgを使用しています。

```bash
cmake -S . -B build \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkgのパス>/scripts/buildsystems/vcpkg.cmake

cmake --build build --config Release
```

依存ライブラリは [vcpkg.json](vcpkg.json) にまとめています。

---

## 開発用Debug Editor

開発用Editorを使用する場合は、実行時に `--debug` を指定します。

```bash
<実行ファイル名> --debug
```

Debug Modeでは、3D Stage Editor、UI Editor、自由Camera、
Stage / UI Data再読み込み、Parameter調整、Asset確認、Performance計測などを利用できます。

---

## 詳細資料

制作背景・コンセプト、開発期間、制作人数、担当範囲、
ゲームデザイン、技術選定理由、開発用Editor、システム設計、
技術的に工夫した点、ビルド・配布については、
[開発要点書（summary.pdf）](summary.pdf) にまとめています。
