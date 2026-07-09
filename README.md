# Slime's Space Travel

## 概要

Slime's Space Travelは、球体地形を探索しながら、ロケットのパーツを集めたり、敵を倒したりしてロケットを出現させ、複数の惑星を旅するステージクリア型3Dアクションゲームです。

C++ / OpenGL を中心に、Bullet Physics、Assimp、SDL2、yaml-cpp、Dear ImGui などを使用して制作しています。Unity や Unreal Engine などの汎用ゲームエンジンは使用していません。

詳細は [summary.pdf](summary.pdf) にまとめています。
この README では、実行方法と、ソースコード上で特に見ていただきたい実装箇所を案内します。

## デモ・資料

* プレイ映像: [https://youtu.be/Xqg5LFoFi6A](https://youtu.be/Xqg5LFoFi6A)
* Releases: https://github.com/kusaka0914/SlimesSpaceTravel/releases
* 要点PDF: [summary.pdf](summary.pdf)

## 実行方法

実行ファイル一式は GitHub Releases からダウンロードできます。

Releases には、実行ファイル、必要なライブラリ、assets、shaders をまとめた配布用ファイルを公開しています。
ダウンロード後、zip ファイルを展開し、展開したフォルダ内の実行ファイルを起動してください。

## ソースからのビルドについて

ビルドはCMake を使用しています。
依存ライブラリは [vcpkg.json](vcpkg.json) にまとめています。

vcpkg を使用する場合は、環境に合わせて toolchain file を指定して CMake を実行してください。

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkgのパス>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

動作を確認だけで良い場合は、GitHub Releases の配布版を使用してください。

## デバッグモードについて

デバッグエディタやステージ調整機能を使用する場合は、実行時に `--debug` オプションを付けて起動します。

```bash
<実行ファイル名> --debug
```

デバッグモードでは、デバッグエディタ用UI、自由カメラ、ステージデータ再読み込み、UIデータ再読み込みなど、調整用の機能を使用できます。

## 開発環境・使用技術

* C++20 / GLSL
* OpenGL
* GLFW / GLEW
* Bullet Physics
* Assimp
* SDL2 / SDL_mixer / SDL_ttf
* yaml-cpp
* GLM
* Dear ImGui
* ImGuizmo
* CMake

## ディレクトリ構成

```txt
src/
├─ actor/             Player、Enemy、Planet、Platform などのゲームオブジェクト
│  ├─ player/         プレイヤーの入力、移動、戦闘、状態管理、被ダメージ処理
│  ├─ enemy/          敵の状態管理、移動、戦闘、被ダメージ処理
│  └─ planet/         惑星上のActor管理、ステージ進行管理
├─ component/         Actorに追加する再利用可能な機能
├─ gfx/               3D描画、UI描画、Shader、Texture管理
│  ├─ debug/          デバッグエディタ、ステージ編集、ギズモ操作
│  ├─ render3d/       3D描画処理の分割実装
│  └─ ui/             HUD、ポーズメニュー、シーンUIなど
├─ state/             ゲーム進行状態・UI表示状態の管理
├─ system/            Physics、Camera、Audio、Input、データ読み込みなど
│  ├─ actor_loader/   YAMLからのActor生成、配置処理
│  ├─ camera/         プレイヤーカメラ、フォーカスカメラ、自由カメラ
│  ├─ mesh/           モデル・テクスチャ・コリジョン用データ読み込み
│  ├─ physics/        Bullet Physics初期化、地形コリジョン、移動衝突判定
│  └─ scene/          チュートリアル、会話、シーン遷移
├─ thirdParty/        Dear ImGui、ImGuizmoなどの外部ライブラリ
├─ utils/             数学処理などの補助機能
└─ Game.cpp           ゲーム全体の初期化、更新、描画呼び出し、各Systemへの委譲
```

## 特に見ていただきたい実装（ここだけでも！というところを抜粋しています。）

### 1. デバッグエディタによるステージ編集

主に見ていただきたい箇所:

* [StageSelectionController.cpp](src/gfx/debug/stage/StageSelectionController.cpp)

  * `UpdatePickedActorByMouse`
  * `UpdateBoxSelection`
  * `MoveSelectedActorsByDelta`
* [StageGizmoController.cpp](src/gfx/debug/stage/StageGizmoController.cpp)

  * `DrawGizmo`
  * `ApplyGizmoMatrixToActor`
* [StageEditCommandController.cpp](src/gfx/debug/stage/StageEditCommandController.cpp)

  * `RestoreUndo`
  * `DuplicateSelectedKeys`

関連ファイル:

* [StageYamlRepository.cpp](src/gfx/debug/stage/StageYamlRepository.cpp)
* [StageActorCreateService.cpp](src/gfx/debug/stage/StageActorCreateService.cpp)
* [StageActorQuery.cpp](src/gfx/debug/stage/StageActorQuery.cpp)

ステージ制作や調整をしやすくするため、Dear ImGui と ImGuizmo を使ったデバッグエディタを実装しています。

マウスクリックによるActor選択、Shiftを使った複数選択、ドラッグによる範囲選択、ギズモによる移動、Actorの追加、削除、複製、アンドゥ、YAMLへの保存、ステージ再読み込みに対応しています。

見るポイント:

* `UpdatePickedActorByMouse` で、マウスクリック位置からRayを作り、Actorを選択している点
* `UpdateBoxSelection` で、ドラッグ矩形による範囲選択を行っている点
* `MoveSelectedActorsByDelta` で、複数選択中のActorをまとめて移動している点
* `DrawGizmo` で、ImGuizmoを使った移動・回転・拡大縮小を行っている点
* `RestoreUndo` で、保存済みYAMLテキストからアンドゥしている点
* `DuplicateSelectedKeys` で、選択中ActorをYAML上で複製している点

### 2. 球体上での接地判定

主に見ていただきたい箇所:

* [Actor.cpp](src/actor/Actor.cpp)

  * `UpdateUpVec`
  * `UpdateDirectionVectors`
  * `UpdateFallbackUpVec`
* [ActorGroundResolver.cpp](src/actor/ActorGroundResolver.cpp)

  * `CalculateAverageNormal`
  * `CalculateFallbackUpVec`
* [CharacterActor.cpp](src/actor/CharacterActor.cpp)

  * `JudgeLanding`
  * `TryLandByRay`
  * `ApplyGravity`

球体地形や楕円、滑らかな壁の上を自然に移動できるように、足元方向へレイキャストを行い、地面法線を取得してキャラクターの上方向ベクトルを更新しています。

当初は惑星中心からプレイヤー位置へのベクトルを上方向として扱っていましたが、その方法では、惑星から突き出した足場や角度のついた地形で姿勢が不自然になる問題がありました。現在は、中央・前後左右へのレイキャストで地面法線を取得し、その平均法線をもとに上方向を更新することで、球体地形、足場、緩やかな壁、楕円形の地形に対応しています。

見るポイント:

* 中央・前後左右へのレイキャストによる地面法線取得
* 取得した法線をもとにした `upVec` 更新
* `upVec` に応じた前方向・左方向ベクトルの再計算
* 接地判定と重力方向を、キャラクターの上方向に合わせて処理している点
* 地面法線が取得できない場合に、惑星形状に応じたフォールバック方向を使うことで無限に落下することを防いでいる点

### 3. Bullet Physics によるモデル形状コリジョン

主に見ていただきたい箇所:

* [PhysicsSystem.cpp](src/system/PhysicsSystem.cpp)

  * `Initialize`
  * `CheckCollision`
* [StageCollisionBuilder.cpp](src/system/physics/StageCollisionBuilder.cpp)

  * `CreateStageCollisionBodies`
  * `CreateStaticMeshBody`
  * `CreateKinematicMeshBody`
  * `CreateTriangleMesh`
* [MeshCollisionDataLoader.cpp](src/system/mesh/MeshCollisionDataLoader.cpp)

Assimpで読み込んだモデルの頂点・インデックス情報から三角形メッシュを作成し、Bullet Physics の当たり判定として登録しています。

これにより、単純な球体半径による補正では対応できない、突き出した足場や複雑なモデル地形に対して、見た目に近いコリジョンを作成しています。

見るポイント:

* モデルデータから頂点・インデックス情報を取得している点
* `btTriangleMesh` と `btBvhTriangleMeshShape` を使って地形コリジョンを作成している点
* 惑星、通常足場、移動床の形状に合わせて当たり判定を構築している点

### 4. 2人プレイ対応

主に見ていただきたい箇所:

* [InputSystem.cpp](src/system/InputSystem.cpp)

  * `ProcessPlayerJoinInput`
  * `ProcessSceneConfirmInput`
* [Game.cpp](src/Game.cpp)

  * `TryCreatePlayer2`
  * `CreatePlayer2`
* [ActorLoadSystem.cpp](src/system/ActorLoadSystem.cpp)

  * `CreatePlayerFromCurrentStage`
* [CameraSystem.cpp](src/system/CameraSystem.cpp)

  * `GetViews`
* [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)

  * `ResizeState`
  * `UpdateState`

ゲームパッド接続時に、2人目のプレイヤーを現在のステージデータから追加生成できるようにしています。キーボードとゲームパッドで入力プレイヤーを分け、会話や確認入力もプレイヤー番号を考慮して処理しています。

カメラ側では、プレイヤーごとにカメラ状態を持ち、2人プレイ時に複数のビューを扱えるようにしています。

見るポイント:

* `Q` キーで2人目プレイヤーを追加する処理
* 現在ステージのYAMLから2人目プレイヤーを生成する処理
* プレイヤーごとにカメラの状態を管理している点

### 5. 攻撃範囲表示

主に見ていただきたい箇所:

* [PlayerEffectRenderer.cpp](src/gfx/render3d/PlayerEffectRenderer.cpp)

  * `DrawPlayerAttackRange`
  * `DrawEnemyAttackRange`
* [Renderer3D.cpp](src/gfx/Renderer3D.cpp)

  * `DrawAttackRangeVertices`

攻撃方向、攻撃角度、攻撃距離をもとに、攻撃範囲の頂点を動的に生成して描画しています。

地面法線を基準に頂点を生成することで、球体地形や斜面上でも現在の地形に沿った攻撃範囲として表示できるようにしています。これにより、プレイヤーが攻撃の届く範囲を視覚的に把握しやすくなることを目指しました。

見るポイント:

* `DrawPlayerAttackRange` で、攻撃角度・攻撃距離から扇形の頂点を生成している点
* `GL_TRIANGLE_FAN` と `GL_TRIANGLE_STRIP` を使い、半透明の範囲と外周線を描き分けている点
* `DrawEnemyAttackRange` で、敵の攻撃予兆を視覚的に表示している点

## その他可能であれば見ていただきたい実装

### 1. YAMLによるステージ・アクター生成

主に見ていただきたい箇所:

* [ActorLoadSystem.cpp](src/system/ActorLoadSystem.cpp)

  * `LoadData`
  * `CreatePlayerFromStageNode`
  * `CreatePlayerFromCurrentStage`
* [ActorPlacementLoader.cpp](src/system/actor_loader/ActorPlacementLoader.cpp)
* [StageActorFactory.h](src/system/actor_loader/StageActorFactory.h)
* [assets/data/stage](assets/data/stage)

惑星、敵、船、ロケットパーツ、キー、クリスタル、NPC、足場、移動床、落下復帰ポイント、プレイヤーなどをYAMLから読み込み、ステージ上に配置できるようにしています。

コードを書き換えずにステージ構成を変更できるため、ステージ制作や調整を行いやすくしています。Actor生成時には、共通の配置処理を `ActorPlacementLoader` に寄せ、Actorごとの生成処理が重複しすぎないようにしています。

見るポイント:

* ステージ上のオブジェクト配置をYAMLで管理している点
* パラメータ調整やステージ構成の変更を、コード変更なしで行えるようにしている点

### 2. 球体地形向けのカメラ制御

主に見ていただきたい箇所:

* [CameraSystem.cpp](src/system/CameraSystem.cpp)

  * `UpdateCamera`
  * `GetViews`
* [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)

  * `Update`
  * `GetView`
  * `UpdateState`
* [CameraCollisionResolver.cpp](src/system/camera/CameraCollisionResolver.cpp)
* [FocusCamera.cpp](src/system/camera/FocusCamera.cpp)
* [DebugCamera.cpp](src/system/camera/DebugCamera.cpp)

球体地形では、地面法線に応じてプレイヤーの上方向ベクトルが変化します。その値をそのままカメラに反映すると画面が小刻みに揺れるため、カメラの位置や上方向を補間して滑らかに追従するようにしています。

また、カメラと注視点の間に障害物がある場合は、レイキャストによってカメラ位置を補正しています。船、キーなど、状況に応じたフォーカスカメラも実装しています。

見るポイント:

* `upVec` と注視点を補間し、球体地形上でもカメラが急に揺れないようにしている点
* カメラと注視点の間にある障害物を検出し、カメラ位置を補正している点
* 通常カメラ、フォーカスカメラ、自由カメラを状況に応じて切り替えている点
* 2人プレイ時にプレイヤーごとのカメラ状態を管理している点

### 3. 壁衝突判定とスライド移動

主に見ていただきたい箇所:

* [ActorCollisionResolver.cpp](src/system/physics/ActorCollisionResolver.cpp)

  * `CheckCollision`
  * `CheckConflictActors`
  * `CheckConflictWall`

プレイヤーや敵の移動時に、Actor同士の押し戻し判定と、Bullet Physics による壁衝突判定を行っています。

壁に衝突した場合は、衝突位置まで移動したうえで、残りの移動量から壁法線方向の成分を取り除き、壁沿いにスライドするようにしています。スライド方向にも再度 Sweep 判定を行い、二重にめり込みを防いでいます。

見るポイント:

* Enemy、Crystal、NPC との距離ベースの押し戻し判定
* 移動前位置から移動後位置までを `convexSweepTest` で判定している点

### 4. プレイヤーの状態管理・戦闘処理

主に見ていただきたい箇所:

* [Player.cpp](src/actor/Player.cpp)

  * `ApplyConfig`
  * `UpdateActor`
  * `ApplyDamage`
  * `OnBoatArrived`
* [PlayerStateMachine.cpp](src/actor/player/PlayerStateMachine.cpp)
* [PlayerCombat.cpp](src/actor/player/PlayerCombat.cpp)

  * `StartAttacking`
  * `Attack`
  * `StartAfterAttackReaction`
* [PlayerAttackHitDetector.cpp](src/actor/player/PlayerAttackHitDetector.cpp)
* [PlayerAttackResolver.cpp](src/actor/player/PlayerAttackResolver.cpp)

弱攻撃、強攻撃、回避、スペシャル攻撃、敵のガード破壊、打ち上げ、空中攻撃などを実装しています。

リファクタリング後は、`Player` 本体に処理を集中させるのではなく、入力、移動、接地、戦闘、攻撃判定、攻撃結果処理、被ダメージ、ジュエル管理、状態遷移を役割ごとに分離しています。`Player` 本体は外部から呼ばれる窓口として残し、実際の処理は専用クラスに委譲する構成にしています。

見るポイント:

* `PlayerStateMachine` によるアクション状態の切り替え
* `PlayerInput`、`PlayerMovement`、`PlayerCombat` などへの責務分割
* `PlayerAttackResolver` による命中後のダメージ、ブレイク、SE、振動などの処理
* `PlayerDamageHandler` による被ダメージ、ジャスト回避、カウンター処理

### 5. 敵AI・敵の状態管理

主に見ていただきたい箇所:

* [Enemy.cpp](src/actor/Enemy.cpp)

  * `ApplyConfig`
  * `UpdateActor`
  * `ApplyDamage`
  * `ApplyBreak`
* [EnemyStateMachine.cpp](src/actor/enemy/EnemyStateMachine.cpp)
* [EnemyMovement.cpp](src/actor/enemy/EnemyMovement.cpp)
* [EnemyCombat.cpp](src/actor/enemy/EnemyCombat.cpp)
* [EnemyDamageHandler.cpp](src/actor/enemy/EnemyDamageHandler.cpp)

敵は、待機、追跡、攻撃準備、攻撃、ノックバック、死亡などの状態を持ちます。プレイヤーとの距離に応じて追跡や攻撃へ遷移し、攻撃前には予兆SEを再生します。

リファクタリング後は、`Enemy` 本体から状態遷移、移動、攻撃、被ダメージ、HP管理、ブレイクゲージ管理を分離しました。敵の行動仕様を変更する場合に、どのファイルを見ればよいか分かりやすい構成を目指しています。

見るポイント:

* `EnemyStateMachine` が敵の状態を所有している点
* `EnemyMovement` が追跡、攻撃中移動、ノックバック、打ち上げを担当している点
* `EnemyCombat` が敵の攻撃判定とブレイク処理を担当している点
* `EnemyDamageHandler` がダメージ、死亡、ノックバック開始を担当している点

### 6. フィードバックフォームへの導線

主に見ていただきたい箇所:

* [Game.cpp](src/Game.cpp)

  * `OpenFeedbackForm`
* [PauseMenuController.cpp](src/system/PauseMenuController.cpp)

  * `ExecuteSelectedItem`

プレイテスト後に感想を集めやすくするため、ゲーム内メニューからGoogle Formsの感想フォームを開けるようにしています。

実装自体は大きな機能ではありませんが、プレイヤーがフォームの存在を忘れにくくなり、操作感やUIに関するフィードバックを集めやすくするための工夫として実装しました。

見るポイント:

* `ExecuteSelectedItem` でポーズメニューの選択項目に応じた処理を呼び分けている点
* `SDL_OpenURL` を使い、ゲーム外の感想フォームへ遷移できるようにしている点

## 設計で意識したこと

* 処理の役割ごとにクラス・ファイルを分割する
* 外部から呼ばれる自然な窓口は `Game`、`Player`、`Enemy` などに残し、内部処理は専用クラスへ委譲する
* 関数名・クラス名から処理内容が分かるように命名する
* 複雑な条件式は意味のある変数に分ける
* 再利用可能な機能はComponentとして分離する
* `const` や `constexpr` を活用し、意図しない変更を防ぐ
* YAMLを使い、ステージ情報やUI情報をコードから分離する
* デバッグエディタを用意し、ステージ制作・調整を効率化する

## リファクタリングで改善したこと

以前は、`Player`、`Enemy`、`Game` に多くの処理が集まっていました。現在は、以下のように責務ごとに分割しています。

```txt
Player
├─ PlayerInput
├─ PlayerMovement
├─ PlayerGrounding
├─ PlayerBoatRide
├─ PlayerCombat
├─ PlayerAttackHitDetector
├─ PlayerAttackResolver
├─ PlayerDamageHandler
├─ PlayerJewelGauge
├─ PlayerRespawn
├─ PlayerInteraction
└─ PlayerStateMachine

Enemy
├─ EnemyStateMachine
├─ EnemyMovement
├─ EnemyCombat
├─ EnemyDamageHandler
├─ EnemyStatus
├─ EnemyHealth
└─ EnemyBreakGauge

Game
├─ GameWorld
├─ InputSystem
├─ PauseMenuController
├─ StageFlowController
└─ GamepadRumbleService
```

この分割により、移動、攻撃、接地、被ダメージ、状態遷移、ステージ遷移、入力処理などを、変更理由ごとに追いやすくしました。

## 今後の改善点

現在はゲーム本体の主要な責務分割は進めていますが、さらに改善できる点もあります。

* 敵AIの種類を増やし、敵ごとの行動パターンをより分離しやすい構成にする
* リプレイ性を高めるため、ステージごとのギミックやクリア条件を増やす
* ステージエディタをより触りやすく改善し、機能追加を行う

## 補足

本作品の詳細な制作背景、開発期間、所要時間、制作人数、担当範囲、動作環境、操作方法、制作意図は [summary.pdf](summary.pdf) に記載しています。
