#include "Enemy.hpp"

#include <algorithm>

Enemy::Enemy(EnemyType enemy_type, glm::vec2 enemy_position) : enemyType(enemy_type)
{
    transform.position = glm::vec3(enemy_position, 0.0f);

    collider.transform = &transform;
    collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Level);

    collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, 0.0f);
    collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.0f);
}

void Enemy::update(float dt, const std::vector<Collider> &col_list)
{
    if (state == EnemyState::Dead)
        return;

    updateBehavior(dt);

    if (state == EnemyState::Dead)
        return;

    const float gravity = 9.81f * 8.0f;
    const float max_fall_speed = 35.0f;

    velocity.y = std::max(velocity.y - gravity * dt, -max_fall_speed);

    transform.position += glm::vec3(velocity, 0.0f) * dt;

    grounded = false;

    AABB a_world = collider.getWorldSpaceAABB();
    glm::vec3 correction(0.0f);

    for (Collider const &obstacle : col_list)
    {
        if (!shouldCollide(collider, obstacle))
        {
            continue;
        }

        AABB b_world = obstacle.getWorldSpaceAABB();

        if (checkCollision(a_world, b_world))
        {
            correction += getAABBFixVec(a_world, b_world);
        }
    }

    transform.position += correction;

    if (correction.y > 0.0f && std::abs(correction.y) >= std::abs(correction.x))
    {
        grounded = true;

        if (velocity.y < 0.0f)
        {
            velocity.y = 0.0f;
        }
    }
}

void Enemy::setBehaviorState(EnemyState new_state)
{
    state = new_state;

    if (state == EnemyState::Dead)
    {
        health = 0;
        velocity = glm::vec2(0.0f);

        return;
    }
}

void Enemy::takeDamage(int amount)
{
    if (state == EnemyState::Dead || amount <= 0)
        return;

    health -= amount;

    if (health <= 0)
    {
        setBehaviorState(EnemyState::Dead);
    }
}