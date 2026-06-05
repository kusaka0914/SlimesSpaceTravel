#include "Renderer.h"
#include <GL/glew.h>
#include <SDL_ttf.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

class Game;
class Enemy;
class Shader3D;
class VertexArray;
class Player;
class Planet;
class Actor;
class Boat;

class Renderer3D : public Renderer {
public:
    Renderer3D(Game* game);
    ~Renderer3D();
    void Initialize();
    void Draw() const;

private:
    void DrawGameScreenForSinglePerson(float fbWidth, float fbHeight) const;
    void DrawGameScreenForMultiPerson(float fbWidth, float fbHeight) const;

    void DrawScene(const glm::mat4& viewMat, const glm::mat4& projMat) const;

    void SetUniforms(const glm::mat4& viewMat, const glm::mat4& projMat) const;
    void DrawPlanets(const std::vector<Planet*>& planets) const;
    void TryDrawPlayers() const;
    void TryDrawEnemies(const std::vector<Enemy*>& enemies, const glm::mat4& viewMat) const;
    void TryDrawActorOnPlanets(const std::vector<Planet*>& planets, glm::mat4 viewMat) const;

    template <class ActorType> void TryDrawActors(const std::vector<ActorType*>& actors, bool useOrient = true) const
    {
        if (actors.empty()) {
            return;
        }

        for (ActorType* actor : actors) {
            if (!actor->GetIsActive()) {
                continue;
            }

            DrawActor(actor, useOrient);
        }
    }

    void TryDrawActor(Actor* actor, bool useOrient = true) const;

    void DrawActor(Actor* actor, bool useOrient) const;
    void DrawAttackRange(Player* player) const;
    void DrawEnemyAttackRange(Enemy* enemy) const;
    void DrawAttackRangeVertices(const std::vector<glm::vec3>& vertices, GLenum drawMode, const glm::vec4& color) const;
    void DrawEnemyGuard(const glm::mat4& viewMat, const Enemy* enemy) const;
    void DrawEnemyHp(const glm::mat4& viewMat, const Enemy* enemy) const;
    void StartTransparentDraw() const;
    void EndTransparentDraw() const;

private:
    std::unique_ptr<Shader3D> mShader3DUnique;
    Shader3D* mShader3D;
    GLuint mAttackRangeVAO;
    GLuint mAttackRangeVBO;
};