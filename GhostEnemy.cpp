#include "GhostEnemy.hpp"

glm::vec3 getPlayerPos(const std::vector<Collider> &col_list)
{
    for (auto& col : col_list)
    {
        if (col.layer == CollisionLayer::Player)
        {
            return col.transform->position;
        }
    }

    return glm::vec3(0.0f);
}

GhostEnemy::GhostEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet) : Enemy(EnemyType::Ghost, enemy_position, sheet)
{
    gravity_scale = 0.0f;
    health = 10000000;
    contact_damage = 1000;

    collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, 0.0f);
    collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.0f);

    collider.layer = CollisionLayer::Ghost;
    collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Player);
    dash_wait_d = dash_wait;
}

void GhostEnemy::updateBehavior(float dt, const std::vector<Collider> &col_list)
{
    glm::vec3 player_pos = getPlayerPos(col_list);

    dash_wait_d -= dt;

    if (player_pos == glm::vec3(0.0f)) { return; }

    glm::vec2 to_player = glm::vec2(player_pos - transform.position);

    if (is_dashing)
    {
        dash_duration_d -= dt;
        velocity = chase_dir * dash_speed;

        if (dash_duration_d <= 0.0f)
        {
            is_dashing = false;
            dash_wait_d = dash_wait;

            float ratio = dash_duration_d / dash_duration;
            float ease = ratio * ratio;
            float current_speed = glm::mix(chase_speed, dash_speed, ease);

            velocity = chase_dir * current_speed; 
        }
    }
    else
    {
        dash_wait_d -= dt;

        if (glm::length(to_player) > 0.1f)
        {
            glm::vec2 dir = glm::normalize(to_player);

            if (dash_wait_d <= 0.0f)
            {
                is_dashing = true;
                dash_duration_d = dash_duration;
                chase_dir = dir;
                velocity = chase_dir * dash_speed;
            }
            else if (dash_wait_d <= dash_wait * 0.1f)
            {
                velocity = glm::vec2(0.0f);
            }
            else
            {
                glm::vec2 desired_velocity = dir * chase_speed;
                glm::vec2 diff = desired_velocity - velocity;
                velocity += diff * steer_force * dt;

                if (glm::length(velocity) > chase_speed)
                {
                    velocity = glm::normalize(velocity) * chase_speed;
                }
            }
        }
    }
    
    transform.position += glm::vec3(velocity * dt, 0.0f);
}