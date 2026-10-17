#pragma once

#include <glm/mat4x4.hpp>

class Game;

// UGCの平行投影設定に応じた投影行列を返す。aspectRatioは最小0.01へ補正し、画角は度単位で受け取る。
glm::mat4 CalculateCameraProjection(const Game& game, float aspectRatio, float fieldOfViewDegrees);
