#pragma once

#include <glm/mat4x4.hpp>

class Game;

glm::mat4 CalculateCameraProjection(
    const Game& game,
    float aspectRatio,
    float fieldOfViewDegrees);
