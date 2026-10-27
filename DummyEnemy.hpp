#pragma once

#include "Enemy.hpp"

struct DummyEnemy : public Enemy
{
    explicit DummyEnemy(glm::vec2 enemy_position, SpriteSheet const &sheet);

private:
    void updateBehavior(float elapsed) override;
};