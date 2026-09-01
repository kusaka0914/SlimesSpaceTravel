# Slime's Space Travel

## 概要

Slime's Space Travelは、球体や楕円状の惑星を360度駆け巡り、惑星の表・側面・裏側まで探索しながら、敵との戦闘やさまざまなギミックを攻略して進むステージクリア型3Dアクションゲームです。

プレイヤーは2体のスライムに分身して切り替えながら行動でき、片方でスイッチを押してもう片方の進路を作るなど、惑星の表裏と分身を組み合わせた攻略が特徴です。戦闘では連続攻撃やため攻撃、敵の打ち上げ、空中攻撃などを使い分けます。また、2人プレイや、アクションが苦手な人でも遊びやすい「らくらくスタイル」にも対応しています。

さらに、2Dグリッドと3Dプレビューを組み合わせたステージ作成機能を搭載しており、高さを「だん」で切り替えながら積み木感覚でオリジナルステージを作成し、そのまま実際にプレイできます。

C++ / OpenGL を中心に、Bullet Physics、Assimp、SDL2、yaml-cpp、Dear ImGui などを使用して制作しています。Unity や Unreal Engine などの汎用ゲームエンジンは使用しておらず、描画、物理連携、キャラクター制御、カメラ、ステージ管理、ステージエディタなど、ゲームを構成する主要なシステムを実装しています。

詳細は [summary.pdf](summary.pdf) にまとめています。
この README では、実行方法と、ソースコード上で特に見ていただきたい実装箇所を案内します。

<img width="1280" height="764" alt="スクリーンショット 2026-08-25 124647" src="https://github.com/user-attachments/assets/28071dbf-166d-465a-a26b-8b4e9404c04b" />


<img width="1275" height="744" alt="スクリーンショット 2026-08-31 174140" src="https://github.com/user-attachments/assets/df8a8d36-4e73-4384-b31f-5915db8f8ca3" />


<img width="1280" height="748" alt="スクリーンショット 2026-08-31 174331" src="https://github.com/user-attachments/assets/f668b5d8-a00e-4787-ac57-a98e5bfc6d7c" />



## デモ・資料

* プレイ映像: https://youtu.be/GYYkx0_Y8g8?si=9aH_WY7Vdzd2oqSv
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
* stb_image

## ディレクトリ構成

```txt
src/
├─ actor/             Player、Enemy、Planet、Platform などのゲームオブジェクト
│  ├─ player/         入力、移動、重力制御、戦闘、状態管理、アニメーションなど
│  ├─ enemy/          移動、戦闘、HP・ブレイク、状態管理、行動パターンなど
│  │  └─ behavior/    追跡、近接攻撃、突進などの敵行動をAction単位で管理
│  └─ planet/         惑星上のActor管理、惑星ごとの進行処理
│
├─ animation/         Assimpで読み込んだボーンアニメーションの再生・補間
├─ component/         Actorに追加する再利用可能な機能
├─ effect/            パーティクルや各種演出エフェクト
│
├─ gfx/               3D描画、UI描画、Shader、Texture、描画フロー
│  ├─ debug/          開発用エディタとステージ作成機能
│  │  ├─ stage/       開発用のActor選択、配置、YAML編集、アンドゥなど
│  │  └─ ugc/         プレイヤー向け2Dグリッド式ステージ作成、3Dプレビュー、作品管理
│  ├─ render3d/       Actor・エフェクトなどの3D描画処理
│  └─ ui/             HUD、メニュー、チュートリアルなどのUI描画
│
├─ state/             ゲーム進行や画面状態を表すState
│
├─ system/            ゲーム全体から利用する各種システム・Controller
│  ├─ actor_loader/   YAMLからのActor生成・ステージ構築
│  ├─ camera/         プレイヤー、フォーカス、自由カメラの制御
│  ├─ mesh/           モデル・テクスチャ・コリジョンデータの読み込み
│  ├─ physics/        Bullet Physics、地形コリジョン、移動衝突判定
│  └─ scene/          シーン遷移、会話、チュートリアルなど
│
├─ text/              日本語表示・ルビなどのテキスト処理
├─ thirdParty/        Dear ImGui、ImGuizmo、stb_image など
├─ utils/             数学処理などの共通ユーティリティ
│
└─ Game.cpp           各System・Controllerを保持し、ゲーム全体の処理を調停
```

## 特に見ていただきたい実装（ここだけでも！というところを抜粋しています。）

### 1. 球体・楕円体上の360°移動を成立させる幾何・重力システム

主に見ていただきたい箇所:

* [Planet.cpp](src/actor/Planet.cpp)

  * `CalculateEllipseSurfaceProjection`
  * `ResolveEllipseSurfaceFace`
  * `ArePositionsOnSameSurfaceFace`
* [Actor.cpp](src/actor/Actor.cpp)

  * `UpdateUpVec`
  * `UpdateDirectionVectors`
* [ActorGroundResolver.cpp](src/actor/ActorGroundResolver.cpp)

  * `CalculateAverageNormal`
  * `CalculateFallbackUpVec`
* [PlayerPlanetGravityController.cpp](src/actor/player/PlayerPlanetGravityController.cpp)

  * `Update`
  * `CalculateAirbornePhysicsUpDirection`
  * `OnJumpStarted`

球体や楕円体の表・側面・裏側まで歩けるように、ワールド固定のY軸ではなく、キャラクターごとに変化する上方向 `upVec` を基準として、接地、姿勢、移動方向、重力方向を制御しています。

接地中は、キャラクターの中央・前後左右から地面方向へレイキャストし、取得した地面法線を平均して `upVec` を更新します。これにより、球面だけでなく、惑星上に配置された足場や傾斜した地形でも、地面に沿った姿勢を維持できるようにしています。

また、楕円体では単純に「惑星中心からプレイヤーへの方向」を法線として使用できないため、楕円体表面への投影位置と表面法線を計算しています。楕円体外部の点については、楕円体表面上の最近点を求める制約式を満たすパラメータを二分探索で求め、その最近点から表面法線を計算しています。

空中では、現在位置と各惑星表面との関係から重力を受ける惑星を切り替え、着地するまで上方向と重力方向を滑らかに変化させています。

見るポイント:

* 中央・前後左右のレイキャストから地面法線を求めている点
* 地面法線をもとに `upVec` と前後左右方向を構築している点
* 楕円体表面への投影位置と法線を独自に計算している点
* 球体・楕円体で異なるフォールバック方向を使用している点
* ジャンプ中に近い惑星へ重力対象を切り替えている点
* 楕円体の表・裏・側面を位置から判定できるようにしている点


### 2. 2Dグリッドと3Dプレビューを組み合わせたステージ作成システム

主に見ていただきたい箇所:

* [UGCEditorInteractionController.cpp](src/gfx/debug/ugc/UGCEditorInteractionController.cpp)

  * `HandleUndo`
  * `HandleRedo`
  * `ChangeLayer`
  * `MoveSelectionOnGrid`
  * `UpdateSelectionDrag`
* [UGCPreviewController.cpp](src/gfx/debug/ugc/UGCPreviewController.cpp)

  * `SetEditLayer`
  * `AdjustYaw`
  * `UpdateFocusY`
* [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)

  * `DrawUGCPreviewFrame`

プレイヤー自身がオリジナルステージを作成できるように、2Dグリッドを中心とした編集画面と、作成中のステージを確認する3Dプレビューを実装しています。

高さ方向は「だん」としてレイヤー単位で切り替えられるようにし、グリッド上でActorを配置・移動することで、3D座標を直接入力しなくても積み木に近い感覚でステージを作れるようにしています。

3Dプレビューはゲーム本体とは別のFramebufferへ描画し、プレビュー専用のView・Projection行列を生成しています。編集している高さに合わせて注視位置を補間し、上下からの確認や水平方向の回転にも対応しています。

見るポイント:

* 3D空間を高さレイヤーに分けて編集している点
* グリッドサイズに合わせて選択Actorの移動量をスナップしている点
* マウス操作を3D空間上のドラッグ移動へ変換している点
* Undo / Redoに対応している点
* ゲーム画面とは別のFramebufferへ3Dプレビューを描画している点
* 編集中のレイヤーに合わせて3Dプレビューの注視位置を変化させている点


### 3. 任意の上方向に対応した球体世界向けカメラ

主に見ていただきたい箇所:

* [PlayerCamera.cpp](src/system/camera/PlayerCamera.cpp)

  * `Update`
  * `GetView`
  * `UpdateState`
  * `SnapBehindPlayer`
  * `TransitionToPlayer`
* [CameraCollisionResolver.cpp](src/system/camera/CameraCollisionResolver.cpp)

  * `Resolve`
  * `HasClearLineOfSight`
  * `HasCameraClearance`

通常の3DゲームのようにY軸を常に上方向とすることができないため、プレイヤーの `upVec` を基準としてカメラの向きと姿勢を計算しています。

カメラの前方向を現在の `upVec` に対する接平面へ射影し、その平面上で符号付き角度を計算することで、惑星の表面を移動して上方向が変化しても、プレイヤーに対するカメラの向きを維持できるようにしています。

さらに、移動方向に応じた自動追従、プレイヤー背後への自動整列、分身切り替え時のカメラ状態引き継ぎを実装しています。

地形によるカメラのめり込みは、Bullet Physics のRay Testで注視点との間の障害物を検出して補正しています。また、カメラ候補位置の周囲に十分な空間があるか確認するため、球形状を使ったContact Testも実装しています。

見るポイント:

* `upVec` に対する接平面へカメラ方向を射影している点
* 任意の上方向を回転軸としてカメラ方向を補間している点
* 球面移動時の自動追従・自動整列を行っている点
* 分身切り替え時にカメラ状態を引き継いでいる点
* Ray Testによる地形へのめり込み防止
* 球形状のContact Testによるカメラ候補位置の空間判定


### 4. YAMLで行動を組み替えられるAction型敵AI

主に見ていただきたい箇所:

* [EnemyConfigLoader.cpp](src/actor/enemy/EnemyConfigLoader.cpp)

  * `Parse`
* [EnemyBehaviorController.cpp](src/actor/enemy/behavior/EnemyBehaviorController.cpp)

  * `Configure`
  * `Update`
  * `SelectAction`
  * `SwitchAction`
* [EnemyBehaviorActionFactory.cpp](src/actor/enemy/behavior/EnemyBehaviorActionFactory.cpp)

  * `Create`
* [behavior/actions](src/actor/enemy/behavior/actions)

敵の種類を増やすたびに `Enemy.cpp` に条件分岐を追加するのではなく、「待機」「追跡」「近接攻撃」「扇形攻撃」などの行動をActionとして独立させています。

現在は、

* Idle
* Chase
* MeleeAttack
* FanAttack
* RadialAttack
* TripleChargeAttack

などを個別のActionとして実装しています。

さらに、敵が持つActionの組み合わせをYAMLから読み込めるようにしており、Actionの種類、重み、Action固有のパラメータをデータとして設定できます。

実行時には、その時点で開始可能なActionだけを候補にし、各Actionが返す評価値を重みとして行動を選択します。

見るポイント:

* 敵の行動を `EnemyBehaviorAction` として独立させている点
* FactoryによってAction名から実装クラスを生成している点
* YAMLからAction構成とパラメータを読み込んでいる点
* `CanStart` / `CanContinue` によってActionの実行条件を分離している点
* 実行可能なActionから重み付きで次の行動を選択している点
* Action追加時に既存の敵ロジックを大きく変更しなくてよい構成にしている点


### 5. ゲーム画面上で直接編集できるビジュアルUIエディタ

主に見ていただきたい箇所:

* [UICanvasEditorController.cpp](src/gfx/debug/ui/UICanvasEditorController.cpp)

  * `UpdateCanvasSelection`
  * `DuplicateSelected`
  * `DeleteSelected`
  * `RestoreUndo`
* [UILoadSystem.cpp](src/system/UILoadSystem.cpp)

UIの位置や大きさをソースコード上の数値を変更して調整するのではなく、実際のゲーム画面上でUIを直接選択・編集できるエディタを実装しています。

クリックによる選択、複数選択、ドラッグ範囲選択に加えて、重なったUIを同じ位置でクリックした際には選択対象を順番に切り替えられるようにしています。

選択したUIは移動・回転・拡大縮小でき、複製、削除、Undo、YAMLへの保存にも対応しています。

見るポイント:

* ゲーム画面上のUIとマウス位置から選択対象を判定している点
* 重なった複数のUIから選択対象を巡回できる点
* Ctrl / Shiftを使った複数選択に対応している点
* ドラッグ矩形による範囲選択
* 複数UIの移動・回転・拡大縮小
* 複製、削除、Undo、YAML保存までエディタ上で完結する点


### 6. 編集状態を保持したビルド・再起動システム

主に見ていただきたい箇所:

* [EditorBuildRestartService.cpp](src/system/EditorBuildRestartService.cpp)

  * `ResolveSessionFilePath`
  * `LaunchBuildAndRestartHelper`
  * `ResolveRuntimePaths`
* [DebugEditorSessionController.cpp](src/system/DebugEditorSessionController.cpp)

  * `RequestBuildAndRestart`
  * `RestoreAtStartup`
  * `SavePersistentSession`

デバッグエディタで作業している途中でもコード変更を反映しやすくするため、エディタからビルド・再起動を行い、再起動後に編集状態を復元する仕組みを実装しています。

再起動前に現在のデバッグエディタ状態をファイルへ保存し、別のHelper Processへビルドディレクトリ、実行ファイル、設定、セッションファイルなどを渡します。

再起動したゲームは保存されていたセッションを読み込み、編集状態を復元します。ビルドに失敗した場合は、その情報もエディタへ表示できるようにしています。

また、プロセス起動部分はWindowsとその他のOSで処理を分けています。

見るポイント:

* 再起動前にEditor Sessionを保存している点
* 実行中のゲームとは別のHelper Processを起動している点
* 実行ファイルやビルドディレクトリを実行環境から解決している点
* Windowsでは `CreateProcessW`、その他の環境では `fork` / `execl` を使用している点
* 再起動後に保存していたEditor Sessionを復元している点


## その他可能であれば見ていただきたい実装

### 1. 日本語文章から自動生成するルビ表示システム

主に見ていただきたい箇所:

* [JapaneseRubyGenerator.cpp](src/system/text/JapaneseRubyGenerator.cpp)

  * `Generate`
* [RubyText.h](src/text/RubyText.h)
* [UICustomElementRenderer.cpp](src/gfx/ui/UICustomElementRenderer.cpp)

  * `DrawCustomElement`

子どもでも文章を読みやすくするため、漢字を含む日本語文章から読みを取得し、自動でルビを表示する仕組みを実装しています。

Windows版では `Windows::Globalization::JapanesePhoneticAnalyzer` を利用して文章を解析し、漢字を含む部分について読みを取得します。

取得結果は本文と読みを持つSegmentへ分割し、解析後にSegmentから本文を再構築して、元の文章と一致することも確認しています。

UI描画側では、ルビの有無に応じて通常テキスト描画とルビ付き描画を切り替えています。

見るポイント:

* 日本語部分とその他の文字列を分けて解析している点
* 漢字を含むSegmentだけにルビを付けている点
* 解析結果から本文を再構築し、元の文章との対応を検証している点
* 本文と読みを別々のサイズ・位置で描画している点
* 自動生成できない場合を考慮したエラー処理


### 2. YAMLベースのゲームシーケンスシステム

主に見ていただきたい箇所:

* [SequenceTypes.h](src/system/sequence/SequenceTypes.h)
* [SequenceSystem.cpp](src/system/sequence/SequenceSystem.cpp)

  * `Play`
  * `Update`
  * `ApplyMovementClips`
  * `ApplyEventClip`
* [SequenceLibrary.cpp](src/system/sequence/SequenceLibrary.cpp)

  * `Load`
  * `Save`
  * `Create`
  * `Duplicate`

イベント演出を個別のハードコードで実装するのではなく、時間軸上に複数のClipを配置するシーケンスシステムを実装しています。

現在は、

* Actorの移動
* Actorの表示・非表示
* プレイヤー操作の有効・無効
* カメラシーケンスの再生

をClipとして扱えます。

Actor移動では開始時刻、時間、移動前後の座標、Easingを指定でき、Linear、EaseIn、EaseOut、EaseInOutに対応しています。

シーケンスはYAMLから読み書きでき、作成、複製、名前変更、削除にも対応しています。

見るポイント:

* 異なる種類のイベントを共通の `SequenceClip` として扱っている点
* 開始時刻とDurationから時間軸上の処理を評価している点
* Actor移動へEasingを適用している点
* CameraやPlayer Controlも同じSequence内で扱える点
* プレビュー終了時にActorの状態を元へ戻せるようにしている点


### 3. ボーンアニメーションの再生・補間

主に見ていただきたい箇所:

* [AnimationPlayer.cpp](src/animation/AnimationPlayer.cpp)

  * `Update`
  * `CalculateCurrentPose`
  * `CalculateNodeTransform`
  * `InterpolatePosition`
  * `InterpolateRotation`
  * `InterpolateScale`

Assimpから読み込んだボーン・キーフレーム情報をもとに、キャラクターのボーンアニメーションを再生しています。

現在時刻をアニメーションのTickへ変換し、各ボーンについて前後のKeyframeを探してTransformを補間します。

位置と拡大率は線形補間、回転はQuaternionのSLERPを使用し、Skeletonの親子階層を再帰的に辿って最終的なBone Transformを計算しています。

見るポイント:

* 現在時刻から前後のKeyframeを検索している点
* Position / Scaleの補間
* Quaternion SLERPによるRotation補間
* Skeleton階層を再帰的に計算している点
* inverse bind transformを考慮した最終Bone Transformの生成


### 4. OpenGL Timer Queryを使ったGPUパフォーマンス計測

主に見ていただきたい箇所:

* [GpuDurationTimer.cpp](src/gfx/performance/GpuDurationTimer.cpp)

  * `Begin`
  * `End`
  * `PollCompletedMilliseconds`
* [GameFrameRenderer.cpp](src/gfx/GameFrameRenderer.cpp)

  * `PollGpuPerformanceMeasurements`

描画負荷を調査できるように、CPU側の処理時間だけではなく、OpenGLのTimer Queryを使ってGPU側の実行時間も計測しています。

計測結果をすぐに取得するとGPUの処理完了をCPUが待つ可能性があるため、複数のQuery Slotを用意し、結果が取得可能になったQueryだけを後から回収する構成にしています。

ゲームUI、デバッグエディタUIなどを個別に計測し、フレーム表示待ち時間についても別途記録しています。

見るポイント:

* `GL_TIME_ELAPSED` によるGPU時間計測
* 複数Queryを循環利用している点
* `GL_QUERY_RESULT_AVAILABLE` で完了済みか確認してから結果を取得している点
* CPU時間とGPU時間を分けて計測している点


### 5. 分身・合体と1人 / 2人プレイを共通化したプレイヤー管理

主に見ていただきたい箇所:

* [PlayerConfigurationController.cpp](src/system/PlayerConfigurationController.cpp)

  * `ToggleSplit`
  * `JoinSecondPlayer`
  * `ReturnToSinglePlayer`
  * `UpdateSplitMergeTransition`

1人プレイ時には1体のスライムを2体へ分身させて操作対象を切り替え、2人プレイ時にはそれぞれのプレイヤーが1体ずつ操作できるようにしています。

分身と2人プレイを完全に別の仕組みとして作るのではなく、同じPlayer構造とプレイヤー管理処理を基盤として扱っています。

分身時には単純にワールド座標上で横方向へ移動させるのではなく、現在の `upVec` から分身方向を求め、球体・楕円体表面に沿うように生成位置を補正しています。

見るポイント:

* 分身状態と2人参加状態を共通のControllerで管理している点
* 2人目を現在ステージのPlayer設定から生成している点
* 球体・楕円体表面に沿って分身位置を計算している点
* 分身・合体時の位置とScaleを時間補間している点
* 2体が近い場合のみ合体できるようにしている点


### 6. Sweep判定を使った壁衝突・スライド移動

主に見ていただきたい箇所:

* [ActorCollisionResolver.cpp](src/system/physics/ActorCollisionResolver.cpp)

  * `CheckCollision`
  * `DoesSweepHitBlockingStage`
  * `CheckConflictActors`

移動後の位置だけで衝突を判定するのではなく、移動前から移動後までの経路に対してSweep判定を行い、高速移動時にも壁やActorを通り抜けにくいようにしています。

壁へ衝突した場合は、残りの移動量から衝突法線方向へ入り込む成分を取り除くことで、移動を完全に止めるのではなく壁面に沿ってスライドさせます。

敵との衝突についても、モデル形状から求めた境界や楕円体形状を利用し、移動経路上の衝突を検出しています。

見るポイント:

* 移動開始位置から終了位置までをSweep判定している点
* 衝突法線から壁に入り込む移動成分を除去している点
* 衝突後も残りの移動量を利用してスライドさせている点
* 既に重なっている場合と、新しく衝突する場合を分けて扱っている点
* 敵への高速移動でも通り抜けないよう移動経路を判定している点

## 設計で意識したこと

* 処理の役割ごとにクラス・ファイルを分割し、変更理由が異なる処理を1つのクラスに集めすぎない
* 外部から利用する自然な窓口は `Game`、`Player`、`Enemy` などに残し、具体的な処理は専用クラスへ委譲する
* 関数名・クラス名から処理内容が分かるように命名する
* 複雑な条件式は意味のある変数や関数に分ける
* 複数のActorで利用する機能はComponentとして分離する
* `const` や `constexpr` を活用し、意図しない変更を防ぐ
* YAMLを使い、ステージ情報やUI情報をコードから分離する
* 開発用エディタを用意し、ステージ制作・調整を効率化する

## リファクタリングで改善したこと

以前は、`Player`、`Enemy`、`Game` に多くの処理が集まっていました。現在は、以下のように責務ごとに分割しています。

```txt
Player
├─ PlayerInput
├─ PlayerMovement
├─ PlayerGrounding
├─ PlayerPlanetGravityController
├─ PlayerBoatRide
├─ PlayerCombat
│  ├─ PlayerAttackHitDetector
│  └─ PlayerAttackResolver
├─ PlayerJewelGauge
├─ PlayerStatus
├─ PlayerRespawn
├─ PlayerInteraction
├─ PlayerStateMachine
├─ PlayerParticleEffectController
└─ PlayerAnimationController

Enemy
├─ EnemyStateMachine
├─ EnemyMovement
├─ EnemyCombat
├─ EnemyDamageHandler
├─ EnemyStatus
│  ├─ EnemyHealth
│  └─ EnemyBreakGauge
└─ EnemyBehaviorController
   └─ EnemyBehaviorAction

Game
├─ GameWorld
├─ PauseMenuController
├─ StageFlowController
├─ GamepadRumbleService
├─ GameProgressController
├─ PlayerConfigurationController
├─ UGCModeController
├─ UGCPreviewController
├─ DebugEditorSessionController
├─ InputSystem
├─ PhysicsSystem
├─ CameraSystem
├─ AudioSystem
├─ SceneSystem
├─ ParticleSystem
└─ SequenceSystem
```

この分割により、移動、戦闘、重力制御、状態管理、敵AI、ステージ進行などを変更理由ごとに追いやすくし、機能追加時に既存処理へ与える影響を小さくすることを意識しています。

## 補足

本作品の詳細な制作背景、開発期間、所要時間、制作人数、担当範囲、動作環境、操作方法、制作意図は [summary.pdf](summary.pdf) に記載しています。
