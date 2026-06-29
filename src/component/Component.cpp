#include "Component.h"
#include "actor/Actor.h"

Component::Component(Actor* owner, int updateOrder)
    : mOwner(owner),
      mUpdateOrder(updateOrder)
{
}

Component::~Component() = default;

void Component::Update(float deltaTime) {}

void Component::ProcessInput(const uint8_t* state) {}