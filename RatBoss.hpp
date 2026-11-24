#pragma once

#include "Enemy.hpp"

struct RatBoss : public Enemy
{
    explicit RatBoss(glm::vec2 enemy_position, SpriteSheet const &sheet);

    bool do_summon = false;

private:
    void updateBehavior(float dt, const std::vector<Collider> &col_list) override;
    bool canMove(int direction, float dt, const std::vector<Collider> &col_list) const;
    glm::vec2 chase_dir = glm::vec2(0.0f);

    float dash_speed = 12.0f;

    float dash_wait = 2.0f;
    float dash_wait_d = 0.0f;

    float dash_duration = 0.65f;
    float dash_duration_d = 0.0f;

    bool is_dashing = false;
    bool is_windup = false;
    bool is_summon = false;

    int8_t charges_since_summon = 0;
};