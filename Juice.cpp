#include "Juice.hpp"


void Juice::addTrauma(float intensity, float duration, glm::vec2 dir)
{
    trauma = glm::clamp(trauma + intensity, 0.0f, 1.0f);
    float new_decay = trauma / duration;
    decay_rate = new_decay;

    shake_dir = (glm::length(dir) > 0.0f) ? glm::normalize(dir) : glm::vec2(0.0f);
    rand_shake_scale = 1.0f;
    shake_freq = DEFAULT_SHAKE_FREQ;
}

void Juice::hitShake(float intensity, float duration, glm::vec2 dir, float shake_freq_a)
{
    trauma = glm::clamp(trauma + intensity, 0.0f, 1.0f);
    float new_decay = trauma / duration;
    decay_rate = new_decay;

    shake_dir = (glm::length(dir) > 0.0f) ? glm::normalize(dir) : glm::vec2(0.0f);
    rand_shake_scale = 0.0f;
    shake_freq = shake_freq_a;
}

void Juice::update(float dt)
{
    if (!activeCamera) return;
    if (trauma > 0)
    {
        total_elapsed += dt;
        float shake = trauma * trauma;

        float rand_x = glm::perlin(glm::vec2(total_elapsed * shake_freq * rand_shake_scale, 0.0f)) * SHAKE_AMP;
        float rand_y = glm::perlin(glm::vec2(0.0f, total_elapsed * shake_freq * rand_shake_scale)) * SHAKE_AMP;

        glm::vec2 offset(rand_x, rand_y);
        if (glm::length(shake_dir) > 0.0f)
        {
            offset += shake_dir * (glm::perlin(glm::vec2(total_elapsed * shake_freq)) * SHAKE_AMP);
        }

        activeCamera->shake_extra_pos = glm::vec3(offset * shake, 0.0f);
        trauma = std::max(0.0f, trauma - decay_rate * dt);
    }
    else
    {
        trauma = 0.0f;
        decay_rate = 0.0f;
        activeCamera->shake_extra_pos = glm::vec3(0.0f);
    }
}

void Juice::applyHitStop(float time) 
{
    hitstopTimer = time;
}

float Juice::checkDeltaTime(float elapsed)
{
    hitstopTimer -= elapsed;
    return hitstopTimer > 0.0f ? 0.0f : elapsed;
}