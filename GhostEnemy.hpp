#pragma once

#include "Enemy.hpp"

struct GhostEnemy : public Enemy
{
    explicit GhostEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet);

private:
    void updateBehavior(float dt, const std::vector<Collider> &col_list) override;

    glm::vec2 chase_dir = glm::vec2(0.0f);
    float chase_speed = 4.5f;
    float dash_speed = 10.0f;
    float steer_force = 3.0f;

    float dash_wait = 5.0f;
    float dash_wait_d = 0.0f;

    float dash_duration = 0.4f;
    float dash_duration_d = 0.0f;
    bool is_dashing = false;
};