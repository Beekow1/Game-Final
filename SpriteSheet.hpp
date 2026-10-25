#pragma once

#include "GL.hpp"
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct SpriteSheet {
    struct Row {
        std::string name;
        uint32_t frames = 0; // how many cells in the row are used for the animation
        float fps = 10.0f; // TODO: Adjust if character fps is different
        bool loop = true; 
    };

    struct Info {
        glm::uvec2 cell_size = glm::uvec2(0); // pixel count 
        std::vector< Row > rows;
    };

    SpriteSheet(std::string const &png_path, Info const &info);
    ~SpriteSheet(); 
    SpriteSheet(SpriteSheet const&) = delete; 
    SpriteSheet &operator=(SpriteSheet const &) = delete; 

    GLuint texture = 0;
    glm::uvec2 size = glm::uvec2(0); // full sheet in pixels
    Info info; 
    
    uint32_t row_index(std::string const &name) const; // if no row then throw 
    //texture coordinates of one cell 
    void frame_uvs(uint32_t row, uint32_t frame, glm::vec2 *uv_min, glm::vec2 *uv_max) const; 
};