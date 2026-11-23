#include "MiniMap.hpp"

void MiniMap::drawMiniMap(DrawSprites &draw_sprites) const
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
    }
}