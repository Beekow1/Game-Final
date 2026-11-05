#pragma once

#include "Enemy.hpp"

struct RatEnemy : public Enemy
{
    explicit RatEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet);

    bool explodesOnContact = false;
private:
    void updateBehavior(float dt, const std::vector<Collider> &col_list) override;

    int patrol_direction = 1;
    float patrol_speed = 2.0f;

    bool canMove(int direction, float dt, const std::vector<Collider> &col_list) const;
};
