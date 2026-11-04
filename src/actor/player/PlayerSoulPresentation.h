#pragma once

#include <glm/glm.hpp>

class Player;

// 描画呼出中だけ借用する魂の表示情報。操作対象は演出の進行にかかわらず、切り替え先を指す。
struct PlayerSoulPresentation {
    const Player* controlledPlayer = nullptr;
    float transferProgress = 1.0f;
    glm::vec3 transferStartPosition{0.0f};
    glm::vec3 transferEndPosition{0.0f};
    glm::vec3 transferUpDirection{0.0f, 1.0f, 0.0f};
};
