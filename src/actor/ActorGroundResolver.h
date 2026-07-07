#pragma once

#include <functional>
#include <glm/glm.hpp>

class Game;
class Planet;

class ActorGroundResolver {
public:
    using NormalRejector = std::function<bool(const glm::vec3& hitNormal, const glm::vec3& up)>;
    using CastSucceededCallback = std::function<void()>;

    static glm::vec3 CalculateAverageNormal(Game* game, const glm::vec3& pos, const glm::vec3& upVec,
                                            const glm::vec3& forwardVec, const glm::vec3& leftVec,
                                            const NormalRejector& shouldRejectNormal,
                                            const CastSucceededCallback& onCastSucceeded);

    static glm::vec3 CalculateFallbackUpVec(const Planet* currentPlanet, const glm::vec3& actorPos);
};
