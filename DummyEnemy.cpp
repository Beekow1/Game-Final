#include "DummyEnemy.hpp"

DummyEnemy::DummyEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet)
    : Enemy(EnemyType::Dummy, enemy_position, sheet)
{
    health = 100;

    collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, 0.0f);
    collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.0f);
}

void DummyEnemy::updateBehavior(float dt)
{
    velocity.x = 0.0f;
}