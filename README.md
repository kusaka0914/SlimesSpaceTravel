# Slime's Space Travel

C++ / OpenGLで制作した、個人制作のステージクリア型3Dアクションゲームです。

子どもやゲームに慣れていない人でも迷わず楽しめることを目指し、球体・楕円体の惑星を360°探索するゲームを制作しました。1ボタンでコンボや空中攻撃を楽しめる「らくらくスタイル」、漢字への自動ルビ表示、積み木感覚のステージ作成機能など、操作や理解の負担を減らす工夫を取り入れています。

また、こうした遊びやすさを繰り返し改善できるよう、チュートリアル・UI・3Dステージを編集するための開発ツールも自作しました。Unity / Unreal Engineなどの汎用ゲームエンジンは使用せず、描画、キャラクター制御、カメラ、敵AI、ステージ管理などの主要システムをC++で実装しています。

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
- コンボ、ガード破壊、打ち上げ、空中攻撃、惑星への叩きつけを組み合わせる戦闘
- 1人プレイ / 2人プレイ対応
- 1ボタン連打でコンボや空中追撃ができる、アクションが苦手な人向けの「らくらくスタイル」
- 2Dグリッドと3Dプレビューを組み合わせ、積み木感覚で3Dステージを作れるプレイヤー向けステージ作成機能

<img width="1275" alt="Slime's Space Travel gameplay" src="https://github.com/user-attachments/assets/df8a8d36-4e73-4384-b31f-5915db8f8ca3" />

---

## 目次

- [起動・操作方法](#起動操作方法)
- [コードを見る方へ ― まずこの3箇所](#コードを見る方へ--まずこの3箇所)
- [その他の実装](#その他の実装)
- [設計について・自動テスト](#設計について)
- [使用技術・技術選定理由](#使用技術)
- [Windowsでのビルド](#windowsでのビルド)

---

## 起動・操作方法

### 起動方法

実行ファイル一式はitch.ioからダウンロードできます。

https://kusaka0914.itch.io/slimesspacetravel

ZIPファイルを展開し、`game.exe`を起動してください。

### 基本操作

キーボードまたはゲームコントローラーで操作できます。

| 操作 | キーボード | コントローラー |
| --- | --- | --- |
| 移動 | W / A / S / D | 左スティック |
| カメラ操作 | 矢印キー | 右スティック |
| カメラを正面に合わせる | M | R |
| ジャンプ | Space | A |
| 弱攻撃 | J | Y |
| 強攻撃 | K | X |
| 回避 | U | B |
| 特殊攻撃 | N | L |
| スライムの分裂・合体 | I | ZR |
| 操作するスライムの切り替え | Y | ZL |
| ポーズメニュー | Esc | -（マイナス） |

※ コントローラーのボタン表記はNintendo Switch Proコントローラー方式です。
※ ステージ1クリア後、ゲームコントローラーを接続した上で、ポーズメニューから2人プレイを開始できます

---

## コードを見る方へ ― まずこの3箇所

本作では、ゲームに慣れていない人でも遊びやすい仕組みと、それを繰り返し改善できる開発環境に力を入れました。

その中でも、技術的に工夫した以下の3つの実装をご覧ください。

1. **360°移動** — 惑星のどこでも自然に操作できる移動・姿勢・カメラ制御
2. **プレイヤー向けStage Editor** — 直感的な2D操作で3Dステージを作れる編集システム
3. **Build & Restart** — 遊びやすさを繰り返し調整するための開発支援ツール

### 1. 球体・楕円体上でも自然に操作できる360°移動

#### まず見るコード

- [ActorGroundResolver.cpp](src/actor/ActorGroundResolver.cpp)
  - `CalculateAverageNormal`
- [Actor.cpp](src/actor/Actor.cpp)
  - `UpdateUpVec`
  - `UpdateDirectionVectors`
- [PlayerMovement.cpp](src/actor/player/PlayerMovement.cpp)
  - `UpdateCameraRelativeMovementDirections`

#### 課題

本作では、惑星の表・側面・裏側を探索するため、移動する場所によって地面の向きが変化します。

一般的なワールド座標のY軸を上方向とする処理では、惑星の側面や裏側でキャラクターの姿勢や移動方向、カメラの向きが不自然になります。

そのため、地面の向きが変わっても、プレイヤーが操作方向に迷わず、自然に探索できる仕組みが必要でした。

#### 実装

Bullet PhysicsのRay Testを利用し、キャラクターの中央・前後左右から地面を検出します。

取得した地面法線を平均してキャラクターの上方向 `upVec` を更新し、外積を利用して地面に沿った前後・左右方向を計算します。

プレイヤーの移動では、カメラの向きを考慮して移動方向を求め、入力に応じて移動させています。

これにより、惑星の表から側面、裏側へ移動しても、その場所の地面に合わせてキャラクターの姿勢と操作方向を変えられるようにしました。

#### 関連コード

- [PlayerPlanetGravityController.cpp](src/actor/player/PlayerPlanetGravityController.cpp)
  - 空中での重力方向の制御
- [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)
  - 地面の向きに応じたカメラ制御
- [CameraCollisionResolver.cpp](src/system/camera/CameraCollisionResolver.cpp)
  - カメラと地形の衝突回避

空中では地面を検出できない場合もあるため、惑星の形状や位置関係に応じて重力方向を求め、姿勢が急激に変化しないよう補間しています。

また、カメラもプレイヤーの `upVec` を基準に向きを調整し、惑星の裏側でも探索しやすい視点を維持できるよう工夫しています。

### 2. 積み木感覚で3Dステージを作れるStage Editor

#### まず見るコード

[UGCEditorInteractionController.cpp](src/gfx/debug/ugc/UGCEditorInteractionController.cpp)

`UGCEditorInteractionController::UpdateSelectionDrag`

#### 課題

本作では、ゲームを遊ぶだけでなく、プレイヤー自身がステージを作る楽しさも提供したいと考えました。

しかし、一般的な3DエディターのようにX・Y・Z軸を直接操作する方式では、3D操作に慣れていない人にとって難しく、気軽にステージを作れません。

そのため、複雑な3D操作を覚えなくても、直感的に足場や敵を配置できる仕組みが必要でした。

#### 実装

3D空間の操作を、

- 水平方向：2Dグリッド
- 高さ方向：「だん」

に分け、「1マス動かす」「1だん上げる」といった積み木に近い感覚で編集できるようにしました。

内部では、マウス位置から3D空間へRayを生成し、編集用Planeとの交点を求めています。

その交点から移動量を計算し、グリッドにスナップすることで、2D画面上の操作を3D空間のオブジェクト移動へ変換しています。

また、操作を間違えてもやり直せるよう、Undo / Redoを実装し、編集内容とYAML上の保存データの整合性も管理しています。

#### 関連コード

- [UGCPreviewController.cpp](src/gfx/debug/ugc/UGCPreviewController.cpp)
- [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)
  - `DrawUGCPreviewFrame`

編集内容は別のFramebufferへ描画し、2D編集画面と3Dプレビューを切り替えながら確認できるようにしました。

これにより、3D操作の難しさを減らしながら、プレイヤーが自分の思い描いたステージを作りやすくしています。

### 3. 遊びやすさの改善を支えるBuild & Restart

#### まず見るコード

[EditorBuildRestartService.cpp](src/system/EditorBuildRestartService.cpp)

#### 課題

本作では、操作性やUI、チュートリアルなどを繰り返し調整し、実際に動かして確認することを重視しています。

しかし、C++コードの変更後に動作確認するには、

`Build → ゲーム起動 → Editorを開く → 編集中の場所まで戻る`

という操作が必要でした。

特に、ゲームを終了すると編集していたステージやカメラ位置などの作業状態が失われるため、改善と確認を繰り返す際の負担になっていました。

#### 実装

Editor上からBuild & Restartを実行すると、

1. 現在のEditor状態を保存
2. Helper Processを起動
3. ゲーム本体を終了
4. Helper Process側でBuild
5. Build成功後にゲームを再起動
6. 保存していたEditor状態を復元

という処理を自動で行います。

Buildをゲーム本体とは別Processへ任せることで、実行中のゲームを終了した後でもBuildと再起動を継続できる構成にしました。

Windowsでは `CreateProcessW`、POSIX環境では `fork` / `execl` を利用しています。

これにより、コードの変更後も元の編集状態から作業を再開でき、操作性や演出などを繰り返し改善しやすい開発環境を整えました。

#### 関連コード

- [DebugEditorSessionController.cpp](src/system/DebugEditorSessionController.cpp)
- `tools/editor_restart_helper/`

---

## その他の実装

遊びやすさの向上に関わる機能や、それらを繰り返し改善するための開発ツールに加え、ゲームを支える技術的な実装を紹介します。

### 遊びやすさを実現する機能

<details>
<summary><strong>1ボタンで戦闘を楽しめる「らくらくスタイル」</strong></summary>

#### 主なコード

- [PlayerStateMachineIdle.cpp](src/actor/player/PlayerStateMachineIdle.cpp)
  - `TryStartAssistStrongAttack`
  - `TryStartAssistBrokenEnemyAirCombo`
- [PlayerTargetingAssist.cpp](src/actor/player/PlayerTargetingAssist.cpp)
  - `FindAttackTarget`
  - `FaceTarget`

アクションが苦手な人でも戦闘を楽しめるよう、1ボタンの連打でコンボや打ち上げ後の空中追撃を行える「らくらくスタイル」を実装しました。

内部では、周囲の敵から攻撃対象を選択し、プレイヤーを自動で敵の方向へ向けます。また、敵の状態に応じて攻撃や追撃を補助することで、複雑な操作をしなくても多彩なアクションを楽しめるようにしています。

</details>


<details>
<summary><strong>家族や友人と一緒に楽しめるローカル2人プレイ</strong></summary>

#### 主なコード

- [PlayerInputDeviceRouting.cpp](src/system/PlayerInputDeviceRouting.cpp)
- [RenderViewportController.cpp](src/gfx/render3d/RenderViewportController.cpp)
- [CameraSystem.cpp](src/system/CameraSystem.cpp)
- [PlayerConfigurationController.cpp](src/system/PlayerConfigurationController.cpp)

家族や友人と一緒にゲームを楽しめるよう、画面分割によるローカル2人プレイを実装しました。

プレイヤーごとに入力デバイスとカメラを管理し、`glViewport` で画面を上下に分割して、それぞれの視点からステージを描画しています。

また、各プレイヤーのHPなどのUIも画面ごとに表示し、2人がそれぞれ操作できる構成にしています。

</details>


<details>
<summary><strong>漢字が読めない人にも配慮した自動ルビ表示</strong></summary>

#### 主なコード

- [JapaneseRubyGenerator.cpp](src/system/text/JapaneseRubyGenerator.cpp)
- [RubyText.h](src/text/RubyText.h)
- [UICustomElementRenderer.cpp](src/gfx/ui/UICustomElementRenderer.cpp)

漢字を読むのが難しい子どもでも、UIや会話文を理解しやすいよう、日本語文章から自動でルビを生成する仕組みを実装しました。

Windows版では `Windows::Globalization::JapanesePhoneticAnalyzer` を利用して読みを取得しています。

解析結果から本文を再構築し、元の文章と一致することを確認した場合のみ採用することで、文章が不正に変化することを防いでいます。

</details>

### 遊びやすさの改善を支える開発ツール

<details>
<summary><strong>チュートリアルを編集・プレビューできるTutorial Editor</strong></summary>

#### 主なコード

- [TutorialDebugPanel.cpp](src/gfx/debug/panels/TutorialDebugPanel.cpp)
- [TutorialLibrary.cpp](src/system/tutorial/TutorialLibrary.cpp)

ゲームに慣れていない人へ操作方法を分かりやすく伝えるため、チュートリアルの内容や表示方法を繰り返し調整できるエディターを実装しました。

文章や動画などの表示内容・配置を編集でき、指定したページからプレビューして実際の見え方を確認できます。

これにより、説明の分かりやすさを確認しながら、チュートリアルを調整できるようにしました。

</details>

<details>
<summary><strong>ゲーム画面上で直接編集できるUI Editor</strong></summary>

#### 主なコード

- [UICanvasEditorController.cpp](src/gfx/debug/ui/UICanvasEditorController.cpp)
- [UILoadSystem.cpp](src/system/UILoadSystem.cpp)

プレイヤーに必要な情報を分かりやすく伝えるため、ゲーム画面上でUIの配置や大きさを直接調整できるエディターを実装しました。

UIを選択して移動・回転・拡大縮小できるほか、複製・削除・Undo・YAML保存にも対応しています。

複数選択や範囲選択、重なったUIの選択切り替えも実装し、画面上で見え方を確認しながらUIを調整できるようにしました。

</details>

<details>
<summary><strong>3D空間でステージを直接編集できる開発用Stage Editor</strong></summary>

#### 主なコード

- [StageGizmoController.cpp](src/gfx/debug/stage/StageGizmoController.cpp)
- [StageActorYamlWriter.cpp](src/gfx/debug/stage/StageActorYamlWriter.cpp)

ステージの導線や敵・足場の配置を繰り返し調整できるよう、ゲーム内の3D空間でオブジェクトを直接編集する開発用エディターを実装しました。

Actorを選択し、ImGuizmoによる移動・回転・拡大縮小を行えます。また、編集内容はYAMLに保存でき、コードを変更せずにステージを調整できます。

※ プレイヤー向けの積み木型Stage Editorとは別の、開発者向け機能です。

</details>

### ゲームを支える技術的な実装

<details>
<summary><strong>Sweep判定による壁衝突・スライド移動</strong></summary>

#### 主なコード

- [ActorCollisionResolver.cpp](src/system/physics/ActorCollisionResolver.cpp)

移動中のすり抜けを防ぎ、壁に接触しても自然に移動できるよう、Bullet PhysicsのSweep判定を利用した衝突処理を実装しました。

衝突時には、残りの移動量から壁の法線方向へ入り込む成分を除去し、壁面に沿って移動させます。

</details>

<details>
<summary><strong>YAMLで行動を組み替えられるAction型Enemy AI</strong></summary>

#### 主なコード

- [EnemyBehaviorController.cpp](src/actor/enemy/behavior/EnemyBehaviorController.cpp)
- [EnemyBehaviorAction.h](src/actor/enemy/behavior/EnemyBehaviorAction.h)
- [EnemyBehaviorActionFactory.cpp](src/actor/enemy/behavior/EnemyBehaviorActionFactory.cpp)
- [EnemyConfigLoader.cpp](src/actor/enemy/EnemyConfigLoader.cpp)

敵の行動を追加・調整しやすくするため、「待機」「追跡」「近接攻撃」「扇形攻撃」「突進」などを独立したActionとして実装しました。

各Actionは `CanStart` / `CanContinue` / `Evaluate` を持ち、実行可能なActionの評価値を利用して次の行動を選択します。

使用するActionとパラメータはYAMLから変更でき、敵の行動パターンをコードの変更なしで調整できる構成にしています。

</details>

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

機能追加や改善を繰り返す中で、既存の動作に不具合が生じないよう、データ処理や状態管理に自動テストを導入しました。

主に、ステージデータの読み書き、UGCの編集・Undo / Redo、入力デバイスの割り当て、チュートリアルの進行、操作ガイドの表示条件などを検証しています。

テストはCMake / CTestから実行できる構成にしています。操作感や遊びやすさなど、自動テストだけでは判断できない部分については、実際のプレイテストで確認・改善しています。

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
