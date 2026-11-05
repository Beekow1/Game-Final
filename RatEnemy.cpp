#include "RatEnemy.hpp"

RatEnemy::RatEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet) : Enemy(EnemyType::Rat, enemy_position, sheet)
{
    health = 25;
    contact_damage = 1;

    collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, 0.0f);
    collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.0f);
}

bool RatEnemy::canMove(int direction, float dt, const std::vector<Collider> &col_list) const
{
    const AABB body = collider.getWorldSpaceAABB();
    const float turn_dist = 0.02f + patrol_speed * dt;

    AABB wall_probe = body;

    wall_probe.min.y += 0.01f;

    float ground_x = direction > 0 ? body.max.x : body.min.x;

    if (direction > 0)
    {
        wall_probe.min.x = body.max.x;
        wall_probe.max.x = body.max.x + turn_dist;
    }
    else
    {
        wall_probe.min.x = body.min.x - turn_dist;
        wall_probe.max.x = body.min.x;
    }

    bool isSafe = false;
    bool willBeGrounded = false;
    for (Collider const &obstacle : col_list)
    {
        if (obstacle.layer != CollisionLayer::Level ||
            !shouldCollide(collider, obstacle))
        {
            continue;
        }

        const AABB terrain = obstacle.getWorldSpaceAABB();

        if (checkCollision(wall_probe, terrain))
            return false;

        if (ground_x >= terrain.min.x && ground_x <= terrain.max.x && terrain.max.y >= body.min.y - 0.03f && terrain.max.y <= body.min.y + 0.01f)
        {
            willBeGrounded = true;
        }

        isSafe = isSafe ||
                 wall_probe.min.x >= terrain.min.x &&
                     wall_probe.max.x <= terrain.max.x;
    }

    return isSafe && willBeGrounded;
}

void RatEnemy::updateBehavior(float dt, const std::vector<Collider> &col_list)
{

    if (!grounded)
    {
        velocity.x = 0.0f;
        return;
    }

    if (!canMove(patrol_direction, dt, col_list))
    {
        patrol_direction = -patrol_direction;
    }

    velocity.x = static_cast<float>(patrol_direction) * patrol_speed;
    facing_left = patrol_direction < 0;
}
