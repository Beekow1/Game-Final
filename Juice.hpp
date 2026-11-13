#pragma once
#include "Scene.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <random>
#include <iostream>
#include <glm/gtc/noise.hpp>

struct Juice
{     
    private:
        Scene::Camera *activeCamera = nullptr;

        static constexpr float MIN_TRAUMA = 0.001f;
        static constexpr float DEFAULT_DURATION = 0.5f;
        static constexpr float DEFAULT_SHAKE_FREQ = 30.0f;
        static constexpr float SHAKE_AMP = 2.0f;

        float trauma = 0.0f;
        float decay_rate = 0.0f;
        glm::vec2 shake_dir{0.0f};
        float total_elapsed = 0.0f;
        float shake_freq = DEFAULT_SHAKE_FREQ;
        float rand_shake_scale = 1.0f;
        float hitstopTimer = 0.0f;

        Juice()  {}
        ~Juice() {}
    
    public:
        static Juice& Get()
        {
            static Juice instance;
            return instance;
        }

        void SetCamera(Scene::Camera *cam)
        {
            activeCamera = cam;
        }

        Juice(const Juice&) = delete;
        Juice& operator=(const Juice&) = delete;

        std::mt19937 gen{42};

        float getRand()
        {
            std::uniform_real_distribution<float> dist{-1.0f, 1.0f};
            return dist(gen);
        }

        void addTrauma(float intensity, float duration = DEFAULT_DURATION, glm::vec2 dir = glm::vec2(0.0f));
        void hitShake(float intensity, float duration = DEFAULT_DURATION, glm::vec2 dir = glm::vec2(0.0f), float shake_freq_a = DEFAULT_SHAKE_FREQ);
        void update(float dt);
        void applyHitStop(float time);
        float checkDeltaTime(float elapsed);
};