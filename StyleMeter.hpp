#pragma once
#include <glm/glm.hpp>
#include <iostream>

enum class StyleRank
{
    D = 0,
    C,
    B,
    A,
    S,
    Count
};

struct StyleMeter
{
    void update(float dt);
    void changeRank(int change);
    void addPoints(float x, bool is_grounded, int move);
    void smallStylePause();
    float getMultiplier();

    StyleRank style_rank = StyleRank::D;
    float style_cur = 0.0f;
    float style_max = 1.0f;
    float style_decay = 0.3f;

    float style_pause = 0.7f;
    float small_style_pause = 0.2f;
    float style_pause_timer = 0.0f;

    float air_mult = 1.5f;

    int prev_move;
};