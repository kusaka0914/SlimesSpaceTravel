#include "actor/StageObject.h"

StageObject::StageObject(Game* game)
    : Actor(game)
{
    // 静的配置物は保存された姿勢を使い、接地レイによる上方向の再計算を省く。
    mIsUpVecInitialized = true;
}
