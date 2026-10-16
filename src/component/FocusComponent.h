#pragma once

#include "component/Component.h"

class Actor;

// 注目演出の時間を管理し、演出途中で所有Actorを有効化する。カメラ制御はGame・CameraSystemへ任せる。
class FocusComponent : public Component {
public:
    FocusComponent(Actor* owner, int updateOrder = 100);
    void Update(float deltaTime) override;

    // Gameの注目演出を開始し、タイマーを3秒へ戻す。更新時の残り時間が0〜2秒なら所有Actorを有効化する。
    void StartFocus();

    float GetFocusTimer() const { return mFocusTimer; }

    // タイマーが0〜2秒の範囲ならtrue。演出開始前・終了処理後はfalse。
    bool HasReachedRevealMoment() const;

private:
    void UpdateFocusTimer(float deltaTime);
    void TryShowOwner();
    void TryFinishFocus();

private:
    float mFocusTimer;
};
