#include "StyleMeter.hpp"

void StyleMeter::update(float dt)
{
    if (style_pause_timer > 0.0f)
    {
        style_pause_timer = std::max(0.0f, style_pause_timer - dt);
        return;
    }
    style_cur -= dt * style_decay;

    while (style_cur < 0.0f && static_cast<int>(style_rank) > 0)
    {
        changeRank(-1);
        style_cur += style_max;
    }

    style_cur = glm::clamp(style_cur, 0.0f, style_max);
}

void StyleMeter::addPoints(float x, bool is_grounded, int move)
{
    if (!is_grounded) { x *= air_mult; }

    if (move == prev_move) { x *= 0.5f; } 

    style_cur += x;
    float rankDelta = std::floor(style_cur);
    style_cur -= rankDelta;

    if (rankDelta != 0.0f)
    {
        changeRank((int)rankDelta);
    }

    style_pause_timer = style_pause;
    prev_move = move;
}

void StyleMeter::changeRank(int change)
{
    style_rank = (StyleRank)(glm::clamp((int)style_rank + change, 0, (int)StyleRank::Count - 1));
}

void StyleMeter::smallStylePause()
{
    style_pause_timer = std::max(style_pause_timer ,small_style_pause);
}

float StyleMeter::getMultiplier()
{
    switch(style_rank)
    {
        case StyleRank::D:
        {
            return 0.75f;
        }

        case StyleRank::C:
        {
            return 1.0f;
        }

        case StyleRank::B:
        {
            return 1.5f;
        }

        case StyleRank::A:
        {
            return 2.0f;
        }

        case StyleRank::S:
        {
            return 3.0f;
        }

        default:
            return 1.0f;
    }
}