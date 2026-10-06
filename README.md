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

[UGCEditorInteractionController.cpp](src/gfx/debug/ugc/UGCEditorInteractionController.cpp)

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

#### 関連コード

- [UGCPreviewController.cpp](src/gfx/debug/ugc/UGCPreviewController.cpp)
- [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)
  - `DrawUGCPreviewFrame`

編集内容はゲーム本体とは別のFramebufferへ描画し、2D編集画面と3Dプレビューを切り替えながら確認できます。

### 3. 編集状態を保持したBuild & Restart

#### まず見るコード

[EditorBuildRestartService.cpp](src/system/EditorBuildRestartService.cpp)

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

Windowsでは `CreateProcessW`、POSIX環境では `fork` / `execl` を利用しています。

#### 関連コード

- [DebugEditorSessionController.cpp](src/system/DebugEditorSessionController.cpp)
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

画面操作を必要とせず検証できるデータ処理・状態管理については、自動テストを用意しています。

主な対象は、

- Stage YAMLの読み書き
- UGC Stageデータ
- Undo / Redo履歴
- Enemy / Player設定読み込み
- Player入力デバイス割り当て
- Camera設定
- Stage進行
- UI / Tutorial関連状態
- Performance計測用ロジック

などです。

テストはCMake / CTestから実行できる構成にしています。

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
- [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)

`GL_TIME_ELAPSED` を利用してGPU実行時間を測定しています。

結果取得時にCPUを待たせないよう複数のQuery Slotを循環利用し、`GL_QUERY_RESULT_AVAILABLE` で完了したQueryのみ後から取得します。

</details>

<details>
<summary><strong>ゲーム画面上で直接編集できるUI Editor</strong></summary>

### 主なコード

- [UICanvasEditorController.cpp](src/gfx/debug/ui/UICanvasEditorController.cpp)
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
├─ actor/
│  ├─ player/         Playerの入力・移動・重力・戦闘・状態
│  ├─ enemy/          Enemyの移動・戦闘・状態・AI
│  └─ planet/         惑星上のActor・進行管理
│
├─ animation/         Skeletal Animation
├─ component/         Actor間で再利用する機能
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
- Git
- インターネット接続（初回ビルド時）

### ビルド方法

`build_windows.bat` を実行してください。

初回実行時は必要に応じてvcpkgを自動取得し、`vcpkg.json` に記載された依存ライブラリを取得したうえでWindows x64 Release版をビルドします。

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

プログラムの設計・実装の主体は自身とし、主に以下の用途で利用しています。

- 自身で設計・実装したコードについて、責務分割・依存関係・可読性の観点からレビューを受ける
- 自身で考えた設計案・実装方針について妥当性や別案を確認する
- リファレンス等を調査しても方針を決めきれなかった問題について、解決方法の候補を検討する

プレイヤー向けステージ作成機能などでは、2D操作と3D空間の対応を含む実装方針の検討にも利用しています。

生成AIから提示されたコードをそのまま使用するのではなく、各処理が必要な理由を確認し、必要に応じて公式リファレンス等で仕様を調査したうえで、自身の設計へ採用するかを判断しています。

採用した内容についても自身で実装し、最終的な動作確認を行っています。

</details>

---

## 詳細資料

制作背景・コンセプト、開発期間、制作人数、担当範囲、ゲームデザイン、技術選定理由、開発用Editor、システム設計、技術的に工夫した点などは、以下にまとめています。

[開発要点書（summary.pdf）](summary.pdf)
