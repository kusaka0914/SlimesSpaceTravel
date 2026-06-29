# Slime's Space Travel

## 概要

**Slime's Space Travel** は、球体地形を探索しながら敵と戦い、ロケットのパーツを集めたり、敵を倒したりしてロケットを出現させ、複数の惑星を旅するステージクリア型3Dアクションゲームです。

C++ / OpenGL を中心に、Bullet Physics、Assimp、SDL2、yaml-cpp などを使用して制作しています。
Unity や Unreal Engine などの汎用ゲームエンジンは使用していません。

詳細な作品説明、制作意図、操作方法、技術的な工夫は [summary.pdf](summary.pdf) にまとめています。
このREADMEでは、実行方法と、ソースコード上で特に見ていただきたい実装箇所を案内します。

## デモ・資料

- プレイ映像: [https://youtu.be/JpgFTtJgUL4](https://youtu.be/JpgFTtJgUL4)
- PV: [https://youtu.be/Nl9kpUEs8NE](https://youtu.be/Nl9kpUEs8NE)
- Releases: [https://github.com/kusaka0914/SlimesSpaceTravel/releases](https://github.com/kusaka0914/SlimesSpaceTravel/releases)
- 説明資料: [summary.pdf](summary.pdf)

## 実行方法

実行ファイル一式は GitHub Releases からダウンロードできます。

Releases には、実行ファイル、必要なライブラリ、assets、shaders をまとめた配布用ファイルを公開しています。
ダウンロード後、zipファイルを展開し、展開したフォルダ内の実行ファイルを起動してください。

提出用フォルダでは、実行可能形式を `bin` フォルダにまとめています。

## ソースからのビルドについて

CMake を使用しています。
依存ライブラリは [vcpkg.json](vcpkg.json) にまとめています。

vcpkg を使用する場合は、環境に合わせて toolchain file を指定して CMake を実行してください。

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<vcpkgのパス>/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

すぐに動作を確認したい場合は、GitHub Releases の配布版を使用してください。

## 開発環境・使用技術

- C++20 / GLSL
- OpenGL
- GLFW / GLEW
- Bullet Physics
- Assimp
- SDL2 / SDL_mixer / SDL_ttf
- yaml-cpp
- GLM
- Dear ImGui
- CMake

## ディレクトリ構成

```txt
src/
├─ actor/       Player、Enemy、Planet、Platformなどのゲームオブジェクト
├─ component/   Actorに追加する再利用可能な機能
├─ gfx/         3D描画、UI描画、Shader、Texture管理
├─ state/       ゲーム進行状態・UI表示状態の管理
├─ system/      Physics、Camera、Audio、データ読み込みなど
├─ thirdParty/  Dear ImGuiなどの外部ライブラリ
├─ utils/       数学処理などの補助機能
└─ Game.cpp     ゲーム全体の初期化、更新、描画呼び出し
```

## 特に見ていただきたい実装

### 1. 球体・任意形状地形上での接地判定

- [Actor.cpp](src/actor/Actor.cpp)
  - `UpdateUpVec`
  - `GetAverageNormal`
  - `CastRay`
  - `UpdateDirectionVectors`
- [CharacterActor.cpp](src/actor/CharacterActor.cpp)
  - `JudgeLanding`
  - `TryLandByRay`

球体地形や複雑なモデル地形の上を自然に移動できるように、地面法線を取得し、キャラクターの上方向ベクトルを更新しています。

当初は惑星中心からプレイヤー位置へのベクトルを上方向として扱っていましたが、その方法では突き出した足場や角度のついた地形で姿勢が不自然になる問題がありました。
現在は、足元方向へのレイキャストで地面法線を取得し、その法線をもとに上方向を更新することで、球体地形、足場、緩やかな壁、楕円形の地形に対応しています。

見るポイント:

- 中央・前後左右へのレイキャストによる地面法線取得
- 取得した法線をもとにした `upVec` 更新
- `upVec` に応じた前方向・左方向ベクトルの再計算
- 接地判定と重力方向を、キャラクターの上方向に合わせて処理している点

### 2. Bullet Physics によるモデル形状コリジョン

- [PhysicsSystem.cpp](src/system/PhysicsSystem.cpp)
  - `CreateStageCollisionBodies`
  - `CreateStaticMeshBody`
  - `CreateTriangleMesh`

Assimpで読み込んだモデルの頂点・インデックス情報から三角形メッシュを作成し、Bullet Physics の当たり判定として登録しています。

これにより、単純な球体半径による補正では対応できない、突き出した足場や複雑なモデル地形に対して、見た目に近いコリジョンを作成しています。

見るポイント:

- モデルデータから頂点・インデックス情報を取得している点
- `btTriangleMesh` と `btBvhTriangleMeshShape` を使って地形コリジョンを作成している点
- 惑星や足場の形状に合わせて、静的な当たり判定を構築している点

### 3. 壁衝突判定とスライド移動

- [PhysicsSystem.cpp](src/system/PhysicsSystem.cpp)
  - `CheckCollision`
  - `CheckConflictActors`
  - `CheckConflictWall`

プレイヤー移動時に Sweep 判定で壁との衝突を確認しています。
壁に衝突した場合は、衝突位置まで移動したうえで、残りの移動量から壁法線方向の成分を取り除き、壁沿いにスライドするようにしています。

これにより、壁にめり込まず、かつ完全に停止しすぎない自然な移動を目指しました。

見るポイント:

- 移動前位置から移動後位置までを `convexSweepTest` で判定している点
- 壁に当たった後、残りの移動量から壁法線方向の成分を取り除いている点
- スライド方向にも再度 Sweep 判定を行い、二重にめり込みを防いでいる点

### 4. 球体地形向けのカメラ制御

- [CameraSystem.cpp](src/system/CameraSystem.cpp)
  - `UpdateCamera`
  - `GetPlayerView`
  - `ResolveCameraCollision`
  - `GetDebugCameraView`

球体地形では、地面法線に応じてプレイヤーの上方向ベクトルが変化します。
その値をそのままカメラに反映すると画面が小刻みに揺れるため、カメラの位置や上方向を補間して滑らかに追従するようにしています。

また、カメラと注視点の間に障害物がある場合は、レイキャストによってカメラ位置を補正しています。
デバッグ・調整用に、通常のプレイヤー追従カメラとは別に自由カメラモードも実装しています。

見るポイント:

- `upVec` と注視点を補間し、球体地形上でもカメラが急に揺れないようにしている点
- `ResolveCameraCollision` でカメラと注視点の間にある障害物を検出している点
- 船・キーへのフォーカス、自由カメラなど、状況に応じたビューを切り替えている点

### 5. YAMLによるステージ・アクター生成

- [ActorLoadSystem.cpp](src/system/ActorLoadSystem.cpp)
  - `LoadData`
  - `LoadPlanets`
  - `LoadEnemies`
  - `LoadBoats`
  - `LoadBoatParts`
  - `LoadKeys`
  - `LoadCrystals`
  - `LoadNPCs`
  - `LoadPlatforms`
  - `LoadPlayers`
- [assets/data/stage](assets/data/stage)

惑星、敵、船、ロケットパーツ、キー、クリスタル、NPC、足場、プレイヤーなどをYAMLから読み込み、ステージ上に配置できるようにしています。

コードを書き換えずにステージ構成を変更できるため、ステージ制作や調整を行いやすくしています。

見るポイント:

- ステージ上のオブジェクト配置をYAMLで管理している点
- Actorの種類ごとに読み込み処理を分けている点
- パラメータ調整やステージ構成の変更を、コード変更なしで行えるようにしている点

### 6. プレイヤーの状態管理・戦闘処理

- [Player.cpp](src/actor/Player.cpp)
  - `UpdateAlive`
  - `UpdateIdle`
  - `UpdateWorldVec`
  - `UpdateAttacking`
  - `UpdateCharging`
  - `UpdateStrongAttacking`

弱攻撃、強攻撃、回避、スペシャル攻撃、敵のガード破壊、打ち上げ、空中攻撃などを実装しています。

プレイヤーは状態に応じて、待機、回避、攻撃、チャージ、強攻撃、ノックバックなどの更新処理を切り替えています。
敵側も状態遷移を持ち、追跡、攻撃準備、攻撃、ノックバックなどを切り替えながら行動します。

見るポイント:

- `UpdateAlive` でプレイヤーのアクション状態ごとに更新処理を分岐している点
- `UpdateIdle` で入力に応じて、ジャンプ、回避、攻撃、特殊攻撃などへ遷移している点
- `UpdateWorldVec` で `upVec` に垂直な平面へ前方向を投影し、球体地形上でも地面に沿った移動方向を作っている点
- 敵のガード破壊、打ち上げ、空中攻撃など、戦闘の流れを状態管理で制御している点

### 7. 攻撃範囲表示

- [Renderer3D.cpp](src/gfx/Renderer3D.cpp)
  - `DrawAttackRange`
  - `DrawAttackRangeVertices`
  - `DrawEnemyAttackRange`

攻撃方向、攻撃角度、攻撃距離をもとに、攻撃範囲の頂点を動的に生成して描画しています。

地面法線を基準に頂点を生成することで、球体地形や斜面上でも現在の地形に沿った攻撃範囲として表示できるようにしています。
これにより、プレイヤーが攻撃の届く範囲を視覚的に把握しやすくなることを目指しました。

見るポイント:

- 攻撃角度・攻撃距離から扇形の頂点を動的に生成している点
- `GL_TRIANGLE_FAN` と `GL_TRIANGLE_STRIP` を使い、半透明の範囲と外周線を描き分けている点
- `upVec` を基準にすることで、球体地形や斜面上でも攻撃範囲が地形に沿って表示される点

### 8. デバッグ表示・調整補助

- [Game.cpp](src/Game.cpp)
  - `ProcessGameInput`
  - `ReloadCurrentStage`
- [CameraSystem.cpp](src/system/CameraSystem.cpp)
  - `GetDebugCameraView`
- [Renderer3D.cpp](src/gfx/Renderer3D.cpp)
  - `DrawDebugLabels`
- [UILoadSystem.cpp](src/system/UILoadSystem.cpp)

ステージやUIの調整をしやすくするため、デバッグ表示や自由カメラ、YAMLの再読み込み機能を実装しています。

ゲーム実行中にステージデータやUIデータを再読み込みできるようにし、調整後すぐにゲーム内で確認できるようにしています。
また、デバッグモード時にはオブジェクトの識別に使える表示を行い、自由カメラでステージ全体を確認しやすくしています。

見るポイント:

- `R` キーでステージデータを再読み込みできる点
- `I` キーでUIデータを再読み込みできる点
- `P` キーでデバッグ表示を切り替えられる点
- `L` キーで自由カメラモードを切り替えられる点

### 9. フィードバックフォームへの導線

- [Game.cpp](src/Game.cpp)
  - `ProcessPauseMenuInput`
  - `ExecutePauseMenuItem`
  - `OpenFeedbackForm`

プレイテスト後に感想を集めやすくするため、ゲーム内メニューからGoogle Formsの感想フォームを開けるようにしています。

実装自体は大きな機能ではありませんが、プレイヤーがフォームの存在を忘れにくくなり、操作感やUIに関するフィードバックを集めやすくするための工夫として実装しました。

見るポイント:

- ポーズメニューからフォームを開けるようにしている点
- `SDL_OpenURL` を使い、ゲーム外の感想フォームへ遷移できるようにしている点
- プレイテストのフィードバックを集めやすくするため、ゲーム内に導線を作っている点

## 設計で意識したこと

- 処理の役割ごとにクラス・ファイルを分割する
- 関数名から処理内容が分かるように命名する
- 複雑な条件式は意味のある変数に分ける
- 再利用可能な機能はComponentとして分離する
- `const` や `constexpr` を活用し、意図しない変更を防ぐ
- YAMLを使い、ステージ情報やUI情報をコードから分離する

## 今後の改善点

現在は機能実装を優先したため、`Player` や `ActorLoadSystem` など、一部のクラスに責務が集まっている箇所があります。

今後は、プレイヤーの入力処理・移動処理・戦闘処理・状態管理の分離のようなクラス分離を行い、より保守・拡張しやすい構成に改善したいと考えています。

## 補足

本作品の詳細な制作背景、開発期間、所要時間、制作人数、担当範囲、動作環境、操作方法、制作意図は [summary.pdf](summary.pdf) に記載しています。