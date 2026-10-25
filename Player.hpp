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
    glm::vec2 velocity;
    float base_gravity = 9.81f;
    float run_speed = 15.0f;

    float jump_force = 30.0f;
    JumpState jump_state;

    float jump_buffer_duration = 0.25f;
    float jump_buffer_timer;

    float coyote_duration = 0.1f;
    float coyote_timer;

    float jump_cut_mult = 0.35f;

    float dash_force = 40.0f;

    float apex_low_grav_cutoff = 0.8f;

    float move_lock = 1.0f;
    float elapsed_move_lock_timer = 0.0f;
    float move_lock_time;

    bool jump_not_wasted;

    bool is_grounded;

    float run_acceleration = 12.0f;
    float run_deceleration = 15.0f;
    float turn_around_mult = 1.6f;

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