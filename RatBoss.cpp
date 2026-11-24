#include "RatBoss.hpp"

static glm::vec3 getPlayerPos(const std::vector<Collider> &col_list)
{
    for (auto &col : col_list)
    {
        if (col.layer == CollisionLayer::Player)
        {
            return col.transform->position;
        }
    }

    return glm::vec3(0.0f);
}

RatBoss::RatBoss(glm::vec2 enemy_position, SpriteSheet const &sheet) : Enemy(EnemyType::RatBoss, enemy_position, sheet)
{
    gravity_scale = 1.0f;
    health = 300;
    contact_damage = 1;
    money = 45;

    collider.local_aabb.min = glm::vec3(-0.9f, -0.5f, 0.0f);
    collider.local_aabb.max = glm::vec3(0.9f, 1.0f, 0.0f);

    collider.layer = CollisionLayer::Enemy;
    collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Level);
    dash_wait_d = dash_wait;

    is_boss = true;
}

bool RatBoss::canMove(int direction, float dt, const std::vector<Collider> &col_list) const
{
    const AABB body = collider.getWorldSpaceAABB();
    const float turn_dist = 0.02f + dash_speed * dt;

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
            isSafe = isSafe ||
                     wall_probe.min.x >= terrain.min.x &&
                         wall_probe.max.x <= terrain.max.x;
        }
    }

    return isSafe && willBeGrounded;
}

void RatBoss::updateBehavior(float dt, const std::vector<Collider> &col_list)
{
    velocity.x = 0.0f;

    glm::vec3 player_pos = getPlayerPos(col_list);

    if (player_pos == glm::vec3(0.0f))
    {
        return;
    }

    glm::vec2 to_player = glm::vec2(player_pos - transform.position);

    if (anim.finished)
    {
        if (is_summon)
        {
            is_summon = false;
            do_summon = true;
            dash_wait_d = dash_wait;
            anim.play("Idle", true);
            return;
        }

        if (is_windup)
        {
            is_windup = false;
            is_dashing = true;
            dash_duration_d = dash_duration;
            anim.play("Charge", true);
            return;
        }
    }
    if (is_summon || is_windup)
    {
        return;
    }
    if (is_dashing)
    {
        dash_duration_d -= dt;

        int direction = chase_dir.x < 0.0f ? -1 : 1;
        const bool can_move = canMove(direction, dt, col_list);

        if (dash_duration_d <= 0.0f || !grounded || !can_move)
        {
            is_dashing = false;
            ++charges_since_summon;

            dash_wait_d = dash_wait;
            anim.play("Idle", true);
        }
        else
        {
            velocity.x = float(direction) * dash_speed;
            contact_damage = 2;
        }

        return;
    }

    if (!grounded)
    {
        anim.play("Idle");
        return;
    }

    facing_left = to_player.x < 0.0f;
    dash_wait_d -= dt;

    if (dash_wait_d > 0.0f)
    {
        anim.play("Idle");
        return;
    }
    if (glm::length(to_player) > 19.5f)
    {
        return;
    }
    if (charges_since_summon >= 2)
    {
        charges_since_summon = 0;
        is_summon = true;
        anim.play("Summon", true);
    }
    else
    {
        chase_dir = glm::vec2(
            facing_left ? -1.0f : 1.0f,
            0.0f);

        is_windup = true;
        anim.play("ChargeWindup", true);
    }
}