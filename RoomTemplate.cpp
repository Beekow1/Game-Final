#include "RoomTemplate.hpp"
#include "ProcGen.hpp"

static std::vector<RoomTemplate> room_templates;
static std::array<std::vector<int>, 16> templates_by_doors;

void create_room_templates()
{
    //create toom layouts here
    room_templates = {
    RoomTemplate{
            Doors::Right,
            {
                { PropType::Platform, glm::vec2(40.0f, 15.0f), true },
                { PropType::Platform,  glm::vec2(250.0f, 15.0f), true },
            },
            {
                { EnemyType::Dummy, glm::vec2(320.0f, 166.0f) },
            },
        }
    };
}

void create_templates_with_doors()
{
    for (int i = 0; i < room_templates.size(); i++)
    {
        templates_by_doors[room_templates[i].doors].push_back(i);
    }   
}

std::vector<int> const& get_templates_with_doors(int mask)
{
    return templates_by_doors[mask];
}

RoomTemplate const& get_room_templates(int idx)
{
    return room_templates[idx];
}