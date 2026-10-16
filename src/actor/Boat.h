#pragma once

#include "actor/Actor.h"

#include <glm/glm.hpp>
#include <string>
#include <vector>

class Game;
class FocusComponent;
class BoatArrivalPoint;
class Player;

// プレイヤーの乗船状態と惑星間の移動を管理する。拠点では発射演出とステージ遷移を開始する。
class Boat : public Actor {
public:
    Boat(Game* game);
    void Initialize() override;
    void UpdateActor(float deltaTime) override;

    // 拠点では発射開始時にステージ遷移を依頼し、ステージ内では到着点への移動を開始する。
    void StartTravel();
    void StartFocus();

    // 乗員は非所有参照。同じプレイヤーの重複登録は無視する。
    void BoardPlayer(Player* player);

    // 移動中・未乗船・nullptrではfalseを返し、乗員一覧を変更しない。
    bool UnboardPlayer(Player* player);
    bool HasBoardedPlayer(const Player* player) const;

    void SetDestPlanet(Planet* destPlanet);
    void SetDestStage(int destStage) { mDestStage = destStage; }
    void SetArrivalPoint(BoatArrivalPoint* arrivalPoint);

    // 速度はワールド距離/秒。0.1以上に制限する。
    void SetTravelSpeed(float travelSpeed);

    // 旧設定の所要秒数を現在地と到着位置から速度へ変換する。到着先を設定した後に呼ぶ。
    void SetTravelSpeedFromLegacyDuration(float travelDuration);

    // 到着点がない場合に使う、惑星表面からの追加距離。負値は0に制限する。
    void SetDestMargin(float destMargin);
    void SetLaunchSequenceId(const std::string& sequenceId) { mLaunchSequenceId = sequenceId; }
    void RefreshDestination();

    bool GetIsMoving() const { return mIsMoving; }
    bool HasAppeared() const { return mIsActive; }
    bool ShouldRenderUnavailablePreview() const;

    // 惑星間移動の進捗（0〜1）。拠点の発射演出とは別の値。
    float GetProgress() const { return mProgress; }
    float GetTravelSpeed() const { return mTravelSpeed; }
    float GetDestMargin() const { return mDestMargin; }
    int GetDestStage() const { return mDestStage; }
    const glm::vec3& GetDestPos() const { return mDestPos; }
    const std::string& GetLaunchSequenceId() const { return mLaunchSequenceId; }
    Planet* GetDestPlanet() const { return mDestPlanet; }
    FocusComponent* GetFocusComponent() const { return mFocusComponent; }
    BoatArrivalPoint* GetArrivalPoint() const { return mArrivalPoint; }

private:
    void AddFocusComponent();

    void OnShown() const;

    void UpdateMoving(float deltaTime);
    void UpdateMovement(float deltaTime);
    void UpdateBaseLaunch(float deltaTime);

    void FinishMoving();

    BoatArrivalPoint* ResolveArrivalPoint() const;
    glm::vec3 CalculateDestPos() const;

private:
    Planet* mDestPlanet;

    bool mIsMoving;
    bool mIsLaunchingFromBase = false;
    bool mIsActivePrev;

    int mDestStage;

    float mProgress;
    float mTravelSpeed;
    float mTravelDistance;
    float mTravelledDistance;
    float mDestMargin;

    glm::vec3 mStartPos;
    glm::vec3 mDestPos;
    glm::vec3 mBaseLaunchEndPos{0.0f};
    float mBaseLaunchElapsedSeconds = 0.0f;

    std::string mLaunchSequenceId;

    FocusComponent* mFocusComponent;
    BoatArrivalPoint* mArrivalPoint;
    std::vector<Player*> mBoardedPlayers;
};
