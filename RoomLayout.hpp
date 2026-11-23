#pragma once

#include "Collider.hpp"
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "Enemy.hpp"

struct SpriteSheet;

struct RoomEnemySpawn
{
    EnemyType type = EnemyType::Dummy;
    glm::vec2 feet_px = glm::vec2(0.0f);
    float chance = 1.0f;
};

struct RoomLayout
{
    SpriteSheet const *sheet = nullptr;

    glm::uvec2 size_px = glm::uvec2(0);

    std::vector<AABB> solids;
    std::vector<AABB> hazards;

    glm::vec2 spawn_px = glm::vec2(0.0f);
    glm::vec2 exit_px = glm::vec2(0.0f);

    std::vector<RoomEnemySpawn> enemies;
    std::vector<glm::vec2> item_px;

    glm::vec2 keeper_px = glm::vec2(0.0f);

    std::vector<glm::vec2> light_px;

    bool has_spawn = false;
    bool has_exit = false;
};

RoomLayout load_room_layout(std::string const &path, SpriteSheet const *sheet = nullptr);
