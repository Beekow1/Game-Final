#include "SpriteSheet.hpp"

#include "load_save_png.hpp" 
#include "gl_errors.hpp" 

#include <stdexcept>

SpriteSheet::SpriteSheet(std::string const &png_path, Info const &info_) : info(info_) {
    // load images
    std::vector< glm::u8vec4 > pixels;
    load_png(png_path, &size, &pixels, UpperLeftOrigin);

    // safety checks 
    if (info.cell_size.x <= 0 || info.cell_size.y <= 0)
    {
        throw std::runtime_error("Cell sizes in sheet should be greater than 0 in either dimension for sheet " + png_path + ".");
    }
    if (size.x % info.cell_size.x != 0)
    {
        throw std::runtime_error("Sheet width not divisible by cell size width in " + png_path + ".");
    }
    if (size.y % info.cell_size.y != 0)
    {
        throw std::runtime_error("Sheet height not divisible by cell size height in " + png_path + ".");
    }
    if (info.rows.size() != (size.y / info.cell_size.y))
    {
        throw std::runtime_error("Number of rows does not match sheet to cell size calculations in " + png_path + ".");
    }
    for (auto const& r : info.rows) 
    {
        if (r.frames == 0)
        {
            throw std::runtime_error("Empty row in sprite sheet " + png_path + ".");
        }
        if (r.frames * info.cell_size.x > size.x)
        {
            throw std::runtime_error("Number of frames in row exceeds sprite sheet width in " + png_path + ".'"); 
        }
    }

    // upload images
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0); 
    GL_ERRORS();
}

SpriteSheet::~SpriteSheet() {
    glDeleteTextures(1, &texture);
    texture = 0;
}

uint32_t SpriteSheet::row_index(std::string const &name) const {
    uint32_t row_index = 0;
    for (auto const& r : info.rows) {
        if (r.name == name)
        {
            return row_index;
        }
        row_index++;
    }

    throw std::runtime_error("Row " + name + " was not found"); // exited loop and found nothing
}

void SpriteSheet::frame_uvs(uint32_t row, uint32_t frame, glm::vec2 *uv_min, glm::vec2 *uv_max) const {
    uv_min->x = (frame * info.cell_size.x) / (float) size.x;
    uv_min->y = (row * info.cell_size.y) / (float) size.y; 

    uv_max->x = ((frame + 1) * info.cell_size.x) / (float) size.x; 
    uv_max->y = ((row + 1) * info.cell_size.y) / (float) size.y; 
}