#include "RoomLayout.hpp"
#include "SpriteSheet.hpp"
#include "load_save_png.hpp"

#include <algorithm>
#include <stdexcept>

RoomLayout load_room_layout(std::string const &path, SpriteSheet const *sheet)
{
    constexpr uint32_t Tile = 16;
    constexpr float RatSpawnChance = 0.65f;
    constexpr float ExplodingRatSpawnChance = 0.35f;
    // constexpr float GhostSpawnChance = 0.75f;

    enum class TileKind
    {
        Empty,
        Solid,
        Hazard
    };

    RoomLayout layout;
    layout.sheet = sheet;

    std::vector<glm::u8vec4> pixels;
    load_png(path, &layout.size_px, &pixels, LowerLeftOrigin);

    if (layout.size_px.x % Tile != 0 || layout.size_px.y % Tile != 0)
    {
        throw std::runtime_error("Room layout '" + path + "' is not a multiple of 16px.");
    }

    for (uint32_t y = 0; y < layout.size_px.y; y++)
    {
        for (uint32_t x = 0; x < layout.size_px.x; x++)
        {
            glm::u8vec4 &c = pixels[y * layout.size_px.x + x];
            if (c.a == 255 && c.r == 255 && c.g == 255 && c.b == 255)
            {
                layout.light_px.emplace_back(float(x) + 0.5f, float(y) + 0.5f);

                c = glm::u8vec4(0);
                continue;
            }

            if (c.a == 255 && c.r == 255 && c.g == 2 && c.b == 0)
            {
                layout.enemies.push_back({EnemyType::ExplodingRat, glm::vec2(float(x), float(y) - 8.0f), ExplodingRatSpawnChance});

                c = glm::u8vec4(0);
                continue;
            }
            else if (c.a == 255 && c.r == 255 && c.g == 3 && c.b == 0)
            {
                layout.enemies.push_back({EnemyType::RatBoss, glm::vec2(float(x), float(y) - 8.0f), 1.0f});

                c = glm::u8vec4(0);
                continue;
            }
            else if (c.a == 255 && c.r == 255 && c.g >= 1 && c.g <= 15) // general case
            {
                layout.enemies.push_back({EnemyType::Rat, glm::vec2(float(x), float(y) - 8.0f), RatSpawnChance});

                c = glm::u8vec4(0);
                continue;
            }
        }
    }

    uint32_t tiles_x = layout.size_px.x / Tile;
    uint32_t tiles_y = layout.size_px.y / Tile;

    for (uint32_t ty = 0; ty < tiles_y; ty++)
    {
        TileKind run_kind = TileKind::Empty;
        uint32_t run_start = 0;

        for (uint32_t tx = 0; tx <= tiles_x; tx++)
        {
            TileKind kind = TileKind::Empty;

            if (tx < tiles_x)
            {
                glm::u8vec4 c = pixels[(ty * Tile + Tile / 2) * layout.size_px.x + (tx * Tile + Tile / 2)];
                glm::vec2 feet = glm::vec2(float(tx * Tile + Tile / 2), float(ty * Tile));

                if (c.a == 0)
                {
                    // layout loading goes here
                    // if you add colors, add them here
                }
                else if (c.r == 0 && c.g == 0 && c.b == 0)
                { // black = solid wall
                    kind = TileKind::Solid;
                }
                else if (c.r == 255 && c.g == 128 && c.b == 0)
                { // orange (255, 128, 0) = void / pit / hazard
                    kind = TileKind::Hazard;
                }
                else if (c.r == 0 && c.g == 255 && c.b == 0)
                { // green (0, 255, 0) = spawn point
                    layout.spawn_px = feet;
                    layout.has_spawn = true;
                }
                else if (c.r == 255 && c.g == 0 && c.b == 0)
                { // red (255, 0, 0) = dummy spawn
                    layout.enemies.push_back({EnemyType::Dummy, feet, 1.0f});
                }
                else if (c.r == 255 && c.g == 1 && c.b == 0)
                { // red (255, 0, 0) = dummy spawn
                    layout.enemies.push_back({EnemyType::Rat, feet, RatSpawnChance});
                }
                else if (c.r == 255 && c.g == 255 && c.b == 0)
                {
                } // previously: items
                else if (c.r == 255 && c.g == 0 && c.b == 255)
                { // magenta (255, 0, 255) = shopkeeper / npc spawn
                    layout.keeper_px = feet;
                }
                else if (c.r == 0 && c.g == 0 && c.b == 255)
                { // blue (0, 0, 255) = exit door
                    layout.exit_px = feet;
                    layout.has_exit = true;
                }
                else
                {
                    throw std::runtime_error("Uknown color in room layout at tile (" + std::to_string(tx) + ", " + std::to_string(ty) + ").");
                }
            }

            if (kind != run_kind)
            {
                if (run_kind != TileKind::Empty)
                {
                    AABB box;
                    box.min = glm::vec3(float(run_start * Tile), float(ty * Tile), 0.0f);
                    box.max = glm::vec3(float(tx * Tile), float((ty + 1) * Tile), 0.0f);
                    (run_kind == TileKind::Solid ? layout.solids : layout.hazards).push_back(box);
                }
                run_kind = kind;
                run_start = tx;
            }
        }
    }

    // aabb vertical merge
    auto merge_vertical = [&](std::vector<AABB> &aabbs)
    {
        for (size_t i = 0; i < aabbs.size(); i++)
        {
            bool merged = true;

            while (merged)
            {
                merged = false;

                for (size_t j = i + 1; j < aabbs.size(); j++)
                {
                    AABB &a = aabbs[i];
                    AABB &b = aabbs[j];

                    bool same_x =
                        a.min.x == b.min.x &&
                        a.max.x == b.max.x;

                    bool touching =
                        a.max.y == b.min.y ||
                        b.max.y == a.min.y;

                    if (same_x && touching)
                    {
                        a.min.y = std::min(a.min.y, b.min.y);
                        a.max.y = std::max(a.max.y, b.max.y);

                        aabbs.erase(aabbs.begin() + j);

                        merged = true;
                        break;
                    }
                }
            }
        }
    };

    merge_vertical(layout.solids);
    merge_vertical(layout.hazards);

    // item markers
    auto is_yellow = [&](uint32_t px_x, uint32_t px_y)
    {
        glm::u8vec4 c = pixels[px_y * layout.size_px.x + px_x];
        return c.a != 0 && c.r == 255 && c.g == 255 && c.b == 0;
    };

    for (uint32_t y = 0; y < layout.size_px.y; y++)
    {
        bool in_run = false;
        uint32_t run_start = 0;
        for (uint32_t x = 0; x <= layout.size_px.x; x++)
        {
            bool yellow = false;
            if (x < layout.size_px.x)
            {
                yellow = is_yellow(x, y) && !(y > 0 && is_yellow(x, y - 1));
            }
            if (yellow && !in_run)
            {
                in_run = true;
                run_start = x;
            }
            else if (!yellow && in_run)
            {
                in_run = false;
                layout.item_px.push_back(glm::vec2(0.5f * float(run_start + x), float(y)));
            }
        }
    }

    return layout;
}
