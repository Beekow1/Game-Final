#include "Player.hpp"
#include <algorithm>
#include <cmath>

void Player::move(glm::vec2 input_dir, float elapsed)
{
    if (is_dashing)
    {
        return;
    }
    
    if(input_dir.x != 0.0f)
    {
        last_move = input_dir.x;
    }

    float target_speed = input_dir.x * run_speed;

    bool extra_speed = std::abs(velocity.x) > run_speed && velocity.x * input_dir.x > 0.0f;
    float accel_rate = extra_speed ? run_deceleration : run_acceleration;

    if (!is_grounded)
    {
        accel_rate *= 0.75f;
    }
    else if (input_dir.x == 0.0f)
    {
        accel_rate = ground_braking;
    }

    float turn_mult = (velocity.x * input_dir.x < 0.0f) ? turn_around_mult : 1.0f;
    accel_rate *= move_lock * turn_mult;
    
    float max_change = accel_rate * elapsed;
    velocity.x += std::clamp(target_speed - velocity.x, -max_change, max_change);
}

void Player::updatePlayer(float elapsed)
{
    elapsed = std::min(elapsed, 0.05f);

    i_timer = std::max(0.0f, i_timer - elapsed);

    elapsed_move_lock_timer += elapsed;
    if (elapsed_move_lock_timer < move_lock_time)
    {
        move_lock = elapsed_move_lock_timer / move_lock_time;
    }
    else
    {
        move_lock = 1.0f;
        if (is_dashing)
        {
            is_dashing = false;

            velocity.x = std::clamp(velocity.x, -std::abs(latest_dash_dir.x) * dash_end_speed, std::abs(latest_dash_dir.x) * dash_end_speed);
        }
    }

    attemptJump();

    jumpArc();

    applyPlayerGravity(elapsed);

    transform->position += glm::vec3(velocity, 0.0f) * elapsed;

    if (velocity.y < -max_fall_speed)
    {
        velocity.y = -max_fall_speed;
    }
}

void Player::attemptJump()
{
    bool can_jump = (coyote_timer > 0.0f) || (jump_buffer_timer > 0.0f && is_grounded);

    if (can_jump && jump_not_wasted)
    {

        jump_not_wasted = false;
        jump_buffer_timer = 0.0f;
        coyote_timer = 0.0f;

        if ((latest_dash_dir.y < 0.0f) && is_dashing)
        {
            // magic nmbrs sry
            std::cout << "WAVE DASH " << std::endl;
            velocity.x = glm::sign(latest_dash_dir.x) * wavedash_speed;
            velocity.y = jump_force * 0.7f;
            jump_state = JumpState::Jumping;
            
            is_dashing = false;
            is_grounded = false;

            lockMovement(dash_duration);

            return;
        }

        velocity.y = jump_force;
        jump_state = JumpState::Jumping;
    }
}

void Player::jumpArc()
{
    if (is_dashing)
    {
        return;
    }

    if (velocity.y <= 0.0f)
    {
        jump_state = is_grounded ? JumpState::Grounded : JumpState::Falling;
    }
    else if (jump_state != JumpState::Falling)
    {
        jump_state = velocity.y <= apex_low_grav_cutoff ? JumpState::Apex : JumpState::Jumping;
    }
}

void Player::dash(glm::vec2 input_dir)
{
    if (cur_dash_count <= 0)
    {
        return;
    }

    glm::vec2 dash_dir;
    if (glm::length(input_dir) < 1e-3)
    {
        dash_dir = glm::vec2(last_move, 0.0f);
    }
    else
    {
        dash_dir = glm::normalize(input_dir);
    }

    is_dashing = true;
    lockMovement(dash_duration);

    cur_dash_count--;

    if (is_grounded)
    {
        cur_dash_count++;
    }

    velocity = glm::vec2(0.0f);
    jump_state = JumpState::Jumping;
    velocity += dash_dir * dash_force;
    latest_dash_dir = dash_dir;
}

void Player::applyPlayerGravity(float elapsed)
{
    float current_gravity_mult = 8.0f;

    switch (jump_state)
    {
    case JumpState::Grounded:
        current_gravity_mult = 8.0f;
        break;
    case JumpState::Jumping:
        current_gravity_mult = 10.0f;
        break;
    case JumpState::Apex:
        current_gravity_mult = 6.0f;
        break;
    case JumpState::Falling:
        current_gravity_mult = 12.0f;
        break;
    }
    velocity.y -= base_gravity * current_gravity_mult * elapsed;
}

void Player::attemptJumpCut()
{
    if (velocity.y > 0.0f && jump_state == JumpState::Jumping && !is_dashing)
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

void Player::dashBounce()
{
    is_dashing = false;
    velocity = -latest_dash_dir * dash_bounce_speed;

    if (std::abs(latest_dash_dir.y) < 0.01f)
    {
        velocity.y = dash_bounce_lift;
    }

    is_grounded = false;
    coyote_timer = 0.0f;

    jump_buffer_timer = 0.0f;
    jump_not_wasted = false;

    jump_state = velocity.y > 0.0f ? JumpState::Jumping : JumpState::Falling;

    lockMovement(dash_bounce_control_time);

    i_timer = std::max(i_timer, dash_attack_grace);
}

bool Player::takeDamage(int amount, glm::vec2 damage_source)
{
    if (health <= 0 || amount <= 0 || i_timer > 0.0f)
    {
        return false;
    }

    health = std::max(0, health - amount);
    i_timer = std::max(i_timer, i_frame_time);

    float source_delta = transform->position.x - damage_source.x;
    float push_direction = source_delta < 0.0f ? -1.0f : 1.0f;

    if (std::abs(source_delta) < 0.001f)
    {
        push_direction = velocity.x > 0.0f ? -1.0f : 1.0f;
    }

    velocity.x = push_direction * hit_knockback_speed;
    velocity.y = hurt_knockback_bounce;

    is_dashing = false;
    is_grounded = false;
    coyote_timer = 0.0f;
    jump_buffer_timer = 0.0f;
    jump_not_wasted = false;

    jump_state = JumpState::Jumping;

    lockMovement(damage_control_lock);

    return true;
}

void runCollision(Player &player, const std::vector<Collider> &col_list)
{
    player.is_grounded = false;
    for (size_t i = 0; i < col_list.size(); i++)
    {
        AABB a_world = col_list[i].getWorldSpaceAABB();

        glm::vec3 total_shift(0.0f);

        for (size_t j = i + 1; j < col_list.size(); j++)
        {
            if (!shouldCollide(col_list[i], col_list[j]))
            {
                continue;
            }

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
                player.cur_dash_count = player.max_dash_count;

                if (player.velocity.y < 0.0f)
                {
                    player.velocity.y = 0.0f;
                }
            }
        }
    }
}