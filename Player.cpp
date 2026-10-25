#include "Player.hpp"

void Player::move(glm::vec2 input_dir, float elapsed)
{
    float target_speed = input_dir.x * run_speed;

    float accel_rate;

    accel_rate = (std::abs(target_speed) > 0.01f) ? run_acceleration : run_deceleration;

    if (glm::sign(input_dir.x) != glm::sign(velocity.x)) { accel_rate *= turn_around_mult; }

    accel_rate *= move_lock;

    float speed_dif = target_speed - velocity.x;

    velocity.x += speed_dif * accel_rate * elapsed;
}

void Player::updatePlayer(float elapsed)
{
    if (elapsed > 0.05f)
    {
        elapsed = 0.05f;
    }

    elapsed_move_lock_timer += elapsed;

    if (elapsed_move_lock_timer < move_lock_time)
    {
       move_lock = elapsed_move_lock_timer / move_lock_time;
    }
    else
    {
        move_lock = 1.0f;
    }

    attemptJump();

    jumpArc();

    applyPlayerGravity(elapsed);
    
    
    transform->position += glm::vec3(velocity, 0.0f) * elapsed;
}

void Player::attemptJump()
{
    bool can_jump = (coyote_timer > 0.0f) || (jump_buffer_timer > 0.0f && is_grounded);

    if (can_jump && jump_not_wasted)
    {
        jump_not_wasted = false;
        jump_buffer_timer = 0.0f;
        coyote_timer = 0.0f;
        velocity.y = jump_force;
        jump_state = JumpState::Jumping;
    }
}

void Player::jumpArc()
{
    if (jump_state == JumpState::Jumping && std::abs(velocity.y) < apex_low_grav_cutoff)
    {
        jump_state = JumpState::Apex;
    }
    else if (jump_state == JumpState::Apex && velocity.y <= -apex_low_grav_cutoff)
    {
        jump_state = JumpState::Falling;
    }
}

void Player::dash(glm::vec2 input_dir)
{
    glm::vec2 dash_dir;
    if (input_dir.length() < 1e-3)
    {
        dash_dir = glm::vec2(1.0f, 0.0f);
    }
    else
    {
        dash_dir = glm::normalize(input_dir);
    }

    lockMovement(0.3f);

    velocity = glm::vec2(0.0f);
    jump_state = JumpState::Jumping;
    velocity += dash_dir * dash_force;
}

void Player::applyPlayerGravity(float elapsed)
{
    float current_gravity_mult = 8.0f;

    switch (jump_state) {
        case JumpState::Grounded:
            current_gravity_mult = 8.0f;
            break;
        case JumpState::Jumping:
            current_gravity_mult = 10.0f;
            break;
        case JumpState::Apex:
            current_gravity_mult = 2.0f;
            break;
        case JumpState::Falling:
            current_gravity_mult = 14.0f;
            break;
    }
    velocity.y -= base_gravity * current_gravity_mult * elapsed;
}

void Player::attemptJumpCut()
{
    if (velocity.y > 0.0f && jump_state == JumpState::Jumping)
    {
        velocity.y *= jump_cut_mult;
        jump_state = JumpState::Falling;
    }
}

void Player::lockMovement(float s)
{
    move_lock_time = s; 
    move_lock = 0.0f;
    elapsed_move_lock_timer = 0.0f;
}

void runCollision(Player& player, const std::vector<Collider>& col_list)
{
    player.is_grounded = false;
    for (size_t i = 0; i < col_list.size(); i++)
    {
        AABB a_world = col_list[i].getWorldSpaceAABB();

        glm::vec3 total_shift(0.0f);

        for (size_t j = i + 1; j < col_list.size(); j++)
        {
            AABB b_world = col_list[j].getWorldSpaceAABB();

            if (checkCollision(a_world, b_world))
            {
                total_shift += getAABBFixVec(a_world, b_world);
            }
        }

        col_list[i].transform->position += total_shift;

        if (col_list[i] == player.collider)
        {
            if (total_shift.y > 0.0f && std::abs(total_shift.y) >= std::abs(total_shift.x))
            {
                player.is_grounded = true;
                player.jump_state = JumpState::Grounded;
                player.coyote_timer = player.coyote_duration;

                if (player.velocity.y < 0.0f)
                {
                    player.velocity.y = 0.0f;
                }
            }
        }
    }    
}