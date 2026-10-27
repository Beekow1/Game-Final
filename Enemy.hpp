#pragma once
#include "Collider.hpp"
#include "SpriteAnimator.hpp"

#include <vector>
enum class EnemyType : uint8_t
{
    Dummy = 0,
};

enum class EnemyState : uint8_t
{
    Idle,
    Patrol,
    Attack,
    Search,
    Dead,
};

struct Enemy
{
    Enemy(EnemyType enemy_type, glm::vec2 enemy_position, SpriteSheet const &sheet);

    virtual ~Enemy() = default;

    EnemyType enemyType;
    EnemyState state = EnemyState::Idle;

    Scene::Transform transform;
    Collider collider;

    glm::vec2 velocity = glm::vec2(0.0f);
    int health = 100;
    int contact_damage = 10;
    bool grounded = false;

    SpriteAnimator anim;
    bool facing_left = false;

    void update(float dt, const std::vector<Collider> &col_list);
    void setBehaviorState(EnemyState new_state);
    void takeDamage(int amount);

private:
    virtual void updateBehavior(float dt) = 0;
};