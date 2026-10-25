#pragma once

#include "SpriteSheet.hpp"
#include <glm/glm.hpp>
#include <vector>

// collects sprites from one sheet and draws them all at once 
// world units are game pixels with y being the up direction
struct DrawSprites {
    DrawSprites(SpriteSheet const &sheet, glm::mat4 const &world_to_clip); 
    ~DrawSprites(); 

    void draw(uint32_t row, uint32_t frame, glm::vec2 position, bool flip_x = false);

    struct Vertex {
        glm::vec3 Position;
        glm::u8vec4 Color; 
        glm::vec2 TexCoord; 
    }; 
    static_assert(sizeof(Vertex) == 12 + 4 + 8, "Vertex is packed.");

    SpriteSheet const &sheet; 
    glm::mat4 world_to_clip; 
    std::vector< Vertex > attribs; 
};