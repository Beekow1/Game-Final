#pragma once
#include "Collider.hpp"
#include "Scene.hpp"
#include <iostream>

enum class JumpState
{
    Grounded,
    Jumping,
    Apex,
    Falling
};

struct Player
{
    float last_move = 1.0f;

    Scene::Transform *transform = nullptr;
    Collider collider;

    // config
    float base_gravity = 9.81f;
    float max_fall_speed = 22.0f;

    float run_speed = 8.0f;
    float run_acceleration = 150.0f;
    float run_deceleration = 100.0f;
    float ground_braking = 200.0f;
    float turn_around_mult = 1.6f;

    float jump_force = 24.0f;
    float jump_cut_mult = 0.15f;
    float apex_low_grav_cutoff = 2.5f;

    float jump_buffer_duration = 0.12f;
    float coyote_duration = 0.1f;

    float dash_force = 20.0f;
    size_t max_dash_count = 1;

    int dash_damage = 25;
    float dash_bounce_speed = 8.0f;
    float dash_bounce_lift = 9.0f;
    float dash_bounce_control_time = 0.06f;
    float dash_duration = 0.2f;
    float dash_end_speed = 13.0f;

    float wavedash_speed = 35.0f;

    float move_lock = 1.0f;

    // combat
    int health = 100;
    float i_frame_time = 0.6f;
    float i_timer = 0.0f;

    float dash_attack_grace = 0.15f;

    float hit_knockback_speed = 8.0f;
    float hurt_knockback_bounce = 5.0f;

    float damage_control_lock = 0.06f;
    // states
    glm::vec2 velocity = glm::vec2(0.0f);
    glm::vec2 latest_dash_dir = glm::vec2(0.0f);

    bool is_dashing = false;
    bool jump_not_wasted = false;
    bool is_grounded = false;
    JumpState jump_state;

    size_t cur_dash_count = 0;

    float jump_buffer_timer = 0.0f;
    float coyote_timer = 0.0f;
    float elapsed_move_lock_timer = 0.0f;
    float move_lock_time = 0.0f;

    void move(glm::vec2 input_dir, float elapsed);

    void updatePlayer(float elapsed);

    void applyPlayerGravity(float elapsed);

    void jumpArc();

    void lockMovement(float s);

    void attemptJump();
    void attemptJumpCut();

    void dash(glm::vec2 input_dir);

    bool takeDamage(int amount, glm::vec2 damage_source);
    void dashBounce();
};

void runCollision(Player &player, const std::vector<Collider> &col_list);