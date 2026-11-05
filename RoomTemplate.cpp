#include "RoomTemplate.hpp"
#include "ProcGen.hpp"

static std::vector<RoomTemplate> room_templates;
static std::array<std::vector<int>, 16> templates_by_doors;

void create_room_templates()
{
    room_templates.clear();

    RoomTemplate base{};

    base.colliders = {
        {glm::vec3(240.0f, 465.0f, 0.0f), glm::vec3(272.0f, 480.0f, 0.0f)},
        {glm::vec3(96.0f, 429.0f, 0.0f), glm::vec3(224.0f, 448.0f, 0.0f)},
        {glm::vec3(288.0f, 429.0f, 0.0f), glm::vec3(416.0f, 448.0f, 0.0f)},
        {glm::vec3(16.0f, 365.0f, 0.0f), glm::vec3(128.0f, 384.0f, 0.0f)},
        {glm::vec3(384.0f, 365.0f, 0.0f), glm::vec3(496.0f, 384.0f, 0.0f)},
        {glm::vec3(96.0f, 285.0f, 0.0f), glm::vec3(416.0f, 304.0f, 0.0f)},
        {glm::vec3(16.0f, 205.0f, 0.0f), glm::vec3(128.0f, 224.0f, 0.0f)},
        {glm::vec3(384.0f, 205.0f, 0.0f), glm::vec3(496.0f, 224.0f, 0.0f)},
        {glm::vec3(96.0f, 125.0f, 0.0f), glm::vec3(224.0f, 144.0f, 0.0f)},
        {glm::vec3(288.0f, 125.0f, 0.0f), glm::vec3(416.0f, 144.0f, 0.0f)},
        {glm::vec3(240.0f, 17.0f, 0.0f), glm::vec3(272.0f, 32.0f, 0.0f)},
        {glm::vec3(0.0f, 496.0f, 0.0f), glm::vec3(240.0f, 512.0f, 0.0f)},
        {glm::vec3(272.0f, 496.0f, 0.0f), glm::vec3(512.0f, 512.0f, 0.0f)},
        {glm::vec3(0.0f, 128.0f, 0.0f), glm::vec3(16.0f, 496.0f, 0.0f)},
        {glm::vec3(496.0f, 128.0f, 0.0f), glm::vec3(512.0f, 496.0f, 0.0f)},
        {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(128.0f, 64.0f, 0.0f)},
        {glm::vec3(128.0f, 0.0f, 0.0f), glm::vec3(224.0f, 48.0f, 0.0f)},
        {glm::vec3(288.0f, 0.0f, 0.0f), glm::vec3(384.0f, 48.0f, 0.0f)},
        {glm::vec3(384.0f, 0.0f, 0.0f), glm::vec3(512.0f, 64.0f, 0.0f)}};

    std::vector<std::vector<EnemyDef>> enemy_groups = {{{EnemyType::Rat, glm::vec2(352.0f, 152.0f)}},
                                                       {{EnemyType::Rat, glm::vec2(160.0f, 312.0f)}, {EnemyType::Rat, glm::vec2(352.0f, 312.0f)}},
                                                       {{EnemyType::Rat, glm::vec2(352.0f, 152.0f)}, {EnemyType::Rat, glm::vec2(64.0f, 232.0f)}, {EnemyType::Rat, glm::vec2(448.0f, 392.0f)}},
                                                       {{EnemyType::Rat, glm::vec2(160.0f, 456.0f)}, {EnemyType::Rat, glm::vec2(352.0f, 456.0f)}, {EnemyType::Rat, glm::vec2(256.0f, 312.0f)}}};
    for (int mask = 0; mask < 16; ++mask)
    {
        for (const auto &group : enemy_groups)
        {

            RoomTemplate variant = base;
            variant.doors = mask;
            variant.enemies = group;
            room_templates.push_back(variant);
        }
    }
}

void create_templates_with_doors()
{
    for (int i = 0; i < room_templates.size(); i++)
    {
        templates_by_doors[room_templates[i].doors].push_back(i);
    }
}

std::vector<int> const &get_templates_with_doors(int mask)
{
    return templates_by_doors[mask];
}

RoomTemplate const &get_room_templates(int idx)
{
    return room_templates[idx];
}