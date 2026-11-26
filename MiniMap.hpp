#pragma once

#include "ProcGen.hpp"
#include "DrawSprites.hpp"
#include "Load.hpp"
#include "SpriteSheet.hpp"
#include "data_path.hpp"
#include <set>
#include <utility>

struct MiniMap
{
    std::vector<std::shared_ptr<Room>> active_rooms;

    glm::vec2 origin = glm::vec2(20.0f, 20.0f); 
    float tile_size = 8.0f;

    glm::vec2 extents = glm::vec2(0.0f); 

    void setMiniMapActiveRooms(std::vector<std::shared_ptr<Room>> ar)
    {
        active_rooms = ar;
    }

    uint32_t get_minimap_frame(int doors) const
    {
        uint32_t frame = 0;
        if (doors & Doors::Right) frame |= 1;
        if (doors & Doors::Down)    frame |= 2;
        if (doors & Doors::Left)  frame |= 4;
        if (doors & Doors::Up)  frame |= 8;
        return frame;
    }

    void drawMiniMap(DrawSprites &draw_sprites, DrawSprites &draw_sprites_location, glm::vec3 player_pos) const;
};
