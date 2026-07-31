#include "actor/enemy/EnemyStatus.h"

EnemyStatus::EnemyStatus()
    : mHealth(),
      mBreakGauge(),
      mIsCountered(false),
      mIsBoss(false),
      mIsHit(false),
      mIsStrongAttacked(false),
      mIsJustBeforeAttack(false),
      mCanCountered(false),
      mAttack(20.0f),
      mDetectionRange(6.0f),
      mMoveSpeed(2.0f),
      mKnockBackSpeed(5.0f),
      mAttackSpeed(1.5f),
      mStandByAttackTimer(-1.0f),
      mDefaultStandByAttackTimer(-1.0f),
      mLaunchedTimer(-1.0f),
      mDefaultLaunchedTimer(-1.0f),
      mLaunchHeight(1.2755f),
      mAttackMotionTimer(-1.0f),
      mDefaultAttackMotionTimer(-1.0f),
      mDyingTimer(-1.0f),
      mKnockBackTimer(-1.0f),
      mCanCounteredTimer(-1.0f),
      mKnockBackFrom(0.0f),
      mNearestPlayer(nullptr)
{
}
