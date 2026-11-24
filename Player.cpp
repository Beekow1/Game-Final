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
        if (is_grounded)
        {
            move_sfx_timer -= elapsed;
        }
    }

    if (move_sfx_timer < 0.0f)
    {
        Sound::play(*step_sample, 0.3f);
        move_sfx_timer = move_sfx_time;
    }

    float target_speed = input_dir.x * run_speed;

    bool extra_speed = std::abs(velocity.x) > run_speed && velocity.x * input_dir.x > 0.0f;
    float accel_rate = extra_speed ? run_deceleration : run_acceleration;

    if (!is_grounded)
    {
        accel_rate *= 0.8f;
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

    style_meter.update(elapsed);

    i_timer = std::max(0.0f, i_timer - elapsed);

    if (i_timer <= 0.0f) { is_hurt_flashing = false; } 

    dash_timer = std::max(0.0f, dash_timer - elapsed);
    tech_timer = std::max(0.0f, tech_timer - elapsed);

    elapsed_move_lock_timer += elapsed;
    if (elapsed_move_lock_timer < move_lock_time)
    {
        move_lock = elapsed_move_lock_timer / move_lock_time;
    }
    else
    {
        move_lock = 1.0f;
        tech_state = TechState::Default;
    }

    if (is_dashing && dash_timer <= 0.0f)
    {
        is_dashing = false;
        velocity.y = std::clamp(velocity.y, -dash_end_speed, dash_end_speed);
    }

    is_attacking = is_dashing || tech_state != TechState::Default;

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
        bool in_tech = tech_timer > 0.0f;
        tech_timer = 0.0f;
        jump_not_wasted = false;
        jump_buffer_timer = 0.0f;
        coyote_timer = 0.0f;

        Sound::play(*jump_sample, 0.3f);

        if ((latest_dash_dir.y < 0.0f && std::abs(latest_dash_dir.x) > 0.0f) && in_tech && dash_started_airborne)
        {
            std::cout << "WAVE DASH " << std::endl;
            velocity.x = glm::sign(latest_dash_dir.x) * wavedash_speed;
            velocity.y = jump_force * 0.8f;
            jump_state = JumpState::Jumping;
            
            is_dashing = false;
            is_grounded = false;

            style_meter.smallStylePause();

            lockMovement(wavedash_duration);
            tech_state = TechState::Wave_Dashing;

            return;
        }
        else if ((latest_dash_dir.y < 0.0f && std::abs(latest_dash_dir.x) == 0.0f) && in_tech)
        {
            std::cout << "SUPER JUMP " << std::endl;
            velocity.y = jump_force * 1.4f;
            jump_state = JumpState::Jumping;

            is_dashing = false;
            is_grounded = false;

            style_meter.smallStylePause();

            lockMovement(dash_time);
            tech_state = TechState::Super_Jumping;
            return;
        }
        else if ((std::abs(latest_dash_dir.y) == 0.0f && std::abs(latest_dash_dir.x) > 0.0f) && in_tech)
        {
            std::cout << "DASH JUMP" << std::endl;
            velocity.y = jump_force * 0.65f;
            velocity.x *= 1.25f;
            jump_state = JumpState::Jumping;

            is_dashing = false;
            is_grounded = false;

            style_meter.smallStylePause();

            lockMovement(dash_jump_duration);
            tech_state = TechState::Dash_Jumping;
            return;
        }
        else if ((latest_dash_dir.y < 0.0f && std::abs(latest_dash_dir.x) > 0.0f) && in_tech && !dash_started_airborne)
        {
            std::cout << "SUPER DASH JUMP" << std::endl;
            velocity.y = jump_force * 0.45f;
            velocity.x *= 1.45f;
            jump_state = JumpState::Jumping;

            is_dashing = false;
            is_grounded = false;

            style_meter.smallStylePause();

            lockMovement(dash_jump_duration);
            tech_state = TechState::Super_Dash_Jumping;
            return;
        }

        velocity.y = jump_force;
        jump_state = JumpState::Jumping;
    }
    else if (jump_not_wasted && jump_buffer_timer > 0.0f && !is_grounded && !is_dashing && air_jumps_left > 0)
    {
        air_jumps_left--;
        jump_not_wasted = false;
        jump_buffer_timer = 0.0f;
        velocity.y = jump_force * air_jump_mult;
        jump_state = JumpState::Jumping; // so jump-cut still works
        Sound::play(*jump_sample, 0.3f);
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

    dash_started_airborne = !is_grounded;
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
    dash_timer = dash_time;
    tech_timer = dash_time + dash_tech_grace;
    lockMovement(dash_time);
    tech_state = TechState::Dashing;
    Sound::play(*dash_sample, 0.3f);

    cur_dash_count--;

    if (is_grounded)
    {
        cur_dash_count++;
    }

    if (dash_dir.x != 0.0f && dash_dir.y == 0.0f) { jump_state = JumpState::Dashing; }
    else { jump_state = JumpState::Apex; }

    jump_state = JumpState::Dashing;

    hit_enemies.clear();

    if (dash_dir.y < 0.0f) { tech_state = TechState::Pre_Super_Jumping; }

    velocity = dash_dir * dash_force;
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
        current_gravity_mult = 8.0f;
        break;
    case JumpState::Apex:
        current_gravity_mult = 2.0f;
        break;
    case JumpState::Falling:
        current_gravity_mult = 9.0f;
        break;
    case JumpState::Dashing:
        current_gravity_mult = 0.0f;
        break;
    case JumpState::LowGrav:
        current_gravity_mult = 4.0f;
        break;
    }
    velocity.y -= base_gravity * current_gravity_mult * elapsed;
}

void Player::attemptJumpCut()
{
    if (velocity.y > 0.0f && jump_state == JumpState::Jumping && !is_dashing && tech_state == TechState::Default)
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
    Sound::play(*attack_hit_sample, 0.3f);
    switch (tech_state)
    {
        case TechState::Dashing:
        {
            velocity = -latest_dash_dir * dash_bounce_speed;
            velocity.y = dash_bounce_lift;

            float lock = (std::abs(latest_dash_dir.y) < 0.01f)
                ? horiz_dash_bounce_control_time
                : dash_bounce_control_time;

            postAttackReset(lock, true);

            Juice::Get().addTrauma(0.3f, 0.1f, -velocity);
            Juice::Get().applyHitStop(0.08f);
            style_meter.addPoints(0.4f, is_grounded, (int)TechState::Dashing);
            break;
        }

        case TechState::Wave_Dashing:
        {
            velocity = -latest_dash_dir * dash_bounce_speed * 1.1f;
            velocity.y = dash_bounce_lift * 1.35f;

            postAttackReset(dash_bounce_control_time, true);

            Juice::Get().addTrauma(0.5f, 0.1f, -velocity);
            Juice::Get().applyHitStop(0.12f);
            style_meter.addPoints(0.7f, is_grounded, (int)TechState::Wave_Dashing);
            break;
        }

        case TechState::Dash_Jumping:
        {
            velocity = latest_dash_dir * dash_bounce_speed * 1.25f;
            velocity.y = dash_bounce_lift * 0.75f;

            postAttackReset(dash_bounce_control_time, true);

            Juice::Get().addTrauma(0.5f, 0.1f, -velocity);
            Juice::Get().applyHitStop(0.12f);
            style_meter.addPoints(0.6f, is_grounded, (int)TechState::Dash_Jumping);
            break;
        }

        case TechState::Super_Dash_Jumping:
        {
            velocity = latest_dash_dir * dash_bounce_speed * 1.55f;
            velocity.y = dash_bounce_lift * 0.65f;

            postAttackReset(dash_bounce_control_time, true);

            Juice::Get().addTrauma(0.6f, 0.2f, -velocity);
            Juice::Get().applyHitStop(0.15f);
            style_meter.addPoints(0.7f, is_grounded, (int)TechState::Super_Dash_Jumping);
            break;
        }

        case TechState::Pre_Super_Jumping:
        {
            velocity = latest_dash_dir * dash_bounce_speed * 0.25f;
            velocity.y = dash_bounce_lift * 1.65f;

            postAttackReset(dash_bounce_control_time, true);

            Juice::Get().addTrauma(0.5f, 0.1f, -velocity);
            Juice::Get().applyHitStop(0.12f);
            style_meter.addPoints(0.5f, is_grounded, (int)TechState::Pre_Super_Jumping);
            break;
        }

        case TechState::Super_Jumping:
        {
            velocity = latest_dash_dir * dash_bounce_speed * 0.25f;
            velocity.y = dash_bounce_lift * 2.0f;

            postAttackReset(dash_bounce_control_time, true);

            Juice::Get().addTrauma(0.5f, 0.1f, -velocity);
            Juice::Get().applyHitStop(0.12f);
            style_meter.addPoints(0.5f, is_grounded, (int)TechState::Super_Jumping);
            break;
        }

        case TechState::Default:
            break;
    }
}

int Player::calculateDamage()
{
    switch (tech_state)
    {
        case TechState::Wave_Dashing:
        {
            return (int)std::ceil(dash_damage * 1.5f);
        }

        case TechState::Dash_Jumping:
        {
            return (int)std::ceil(dash_damage * 1.25f);
        }

        case TechState::Super_Dash_Jumping:
        {
            return (int)std::ceil(dash_damage * 1.35f);
        }

        default:
        {
            return dash_damage;
        }
    }
}

void Player::applyItems()
{
    GameManager const &gm = GameManager::Get();
    max_health      = base_max_health + gm.itemCount(ItemId::MaxHP);
    max_dash_count  = base_max_dash_count + size_t(gm.itemCount(ItemId::Dash));
    dash_damage     = base_dash_damage + 10 * gm.itemCount(ItemId::Attack);
    max_air_jumps   = gm.itemCount(ItemId::Jump);
    style_meter.style_decay = base_style_decay / (1.0f + 0.4f * float(gm.itemCount(ItemId::StyleDecay)));
    health = std::min(health, max_health);
}

void Player::postAttackReset(float control_lock_time, bool refund_dash)
{
    is_dashing = false;
    is_grounded = false;

    coyote_timer = 0.0f;
    tech_timer = 0.0f;
    jump_buffer_timer = 0.0f;
    jump_not_wasted = false;

    lockMovement(control_lock_time);
    jump_state = velocity.y > 0.0f ? JumpState::Jumping : JumpState::Falling;

    if (refund_dash)
    {
        cur_dash_count = std::min(max_dash_count, cur_dash_count + 1);
    }

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
    tech_state = TechState::Default;
    coyote_timer = 0.0f;
    tech_timer = 0.0f;
    jump_buffer_timer = 0.0f;
    jump_not_wasted = false;

    jump_state = JumpState::Jumping;

    i_timer = std::max(i_timer, hit_i_frame_time);

    lockMovement(damage_control_lock);
    
    Juice::Get().addTrauma(0.8f, 0.35f, -velocity);
    Juice::Get().applyHitStop(0.4f);

    is_hurt_flashing = true;

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
                player.air_jumps_left = player.max_air_jumps;

                if (player.is_dashing && player.latest_dash_dir.y < 0.0f)
                {
                    player.dash_timer = 0.0f;
                    player.is_dashing = false;
                    player.lockMovement(0.2f);
                    player.velocity.x *= 1.5f;
                }

                if (player.velocity.y < 0.0f)
                {
                    player.velocity.y = 0.0f;
                }
            }
        }
    }
}

void Player::reset(glm::vec2 position)
{
    transform->position = glm::vec3(position, 0.0f);
    velocity = glm::vec2(0.0f);

    health = max_health;
    i_timer = 0.0f;

    is_dashing = false;
    is_attacking = false;
    dash_timer = 0.0f;
    tech_timer = 0.0f;
    cur_dash_count = max_dash_count;
    air_jumps_left = max_air_jumps;

    coyote_timer = 0.0f;
    jump_buffer_timer = 0.0f;
    jump_not_wasted = false;

    move_lock = 1.0f;
    move_lock_time = 0.0f;
    tech_state = TechState::Default;
    jump_state = JumpState::Falling;
}