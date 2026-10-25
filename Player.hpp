#pragma once
#include "Collider.hpp"
#include "Scene.hpp"
#include <iostream>

enum class JumpState{
    Grounded,
    Jumping,
    Apex,
    Falling
};

struct Player
{
    Scene::Transform *transform = nullptr;
    Collider collider;

    // config
    float base_gravity = 9.81f;
    float max_fall_speed = 35.0f;

    float run_speed = 15.0f;
    float run_acceleration = 12.0f;
    float run_deceleration = 15.0f;
    float turn_around_mult = 1.6f;

    float jump_force = 30.0f;
    float jump_cut_mult = 0.35f;
    float apex_low_grav_cutoff = 0.8f;

    float jump_buffer_duration = 0.25f;
    float coyote_duration = 0.1f;
    
    float dash_force = 40.0f;
    size_t max_dash_count = 1;

    float move_lock = 1.0f;

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
};

void runCollision(Player& player, const std::vector<Collider>& col_list);