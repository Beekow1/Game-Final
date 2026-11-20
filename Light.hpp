#pragma once

#include <glm/glm.hpp>

struct Light {
    glm::vec2 position = glm::vec2(0.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float radius = 8.0f;
    float intensity = 1.0f;
    bool do_march = true;
};