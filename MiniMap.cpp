#include "MiniMap.hpp"

void MiniMap::drawMiniMap(DrawSprites &draw_sprites, DrawSprites &draw_sprites_location, glm::vec3 player_pos) const
{
    std::set<std::pair<int, int>> drawn;

    auto const &rooms_row = draw_sprites.sheet.row_index("Rooms");

    for (const auto &room : active_rooms)
    {
        if (!room) continue;

        if (!drawn.insert({room->grid_x, room->grid_y}).second) continue;

        glm::vec2 pos = origin + glm::vec2(room->grid_x, room->grid_y) * tile_size;
        uint32_t frame = get_minimap_frame(room->doors);

        draw_sprites.draw(rooms_row, frame, pos, false);

        glm::vec2 player_room_offset = glm::vec2(room->grid_x - active_rooms[0]->grid_x, room->grid_y - active_rooms[0]->grid_y);

        if (player_room_offset.x == (int)std::floorf(player_pos.x / 32.0f) && player_room_offset.y == (int)std::floorf(player_pos.y / 32.0f))
        {
            glm::vec2 heart_pos = pos * (1.75f / 0.9f);
            draw_sprites_location.draw(draw_sprites_location.sheet.row_index("Hearts"), 0, heart_pos, false);
        }
    }
}