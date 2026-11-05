#pragma once
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include "Enemy.hpp"

enum class PropType { Platform, Barrels, Shelf, DecoOne, DecoTwo };
//enum class EnemyType { Dummy };

struct PropDef  { PropType type; glm::vec2 local_px; bool is_fg; };
struct EnemyDef { EnemyType type; glm::vec2 local_px; };    

struct RoomTemplate
{
    int doors;
    std::vector<PropDef>  props;
    std::vector<EnemyDef> enemies;
};

void create_room_templates();
void create_templates_with_doors();

std::vector<int> const& get_templates_with_doors(int mask);

RoomTemplate const& get_room_templates(int idx);