#include "SpriteAnimator.hpp"
#include <algorithm>
#include <cmath>

SpriteAnimator::SpriteAnimator(SpriteSheet const &sheet_, std::string const &start_row)
	: sheet(sheet_), row(sheet_.row_index(start_row)) {
}

void SpriteAnimator::play(std::string const &name, bool restart) {
	uint32_t new_row = sheet.row_index(name);
	if (new_row == row && !restart) return; //already playing
	row = new_row;
	time = 0.0f;
	finished = false;
}

void SpriteAnimator::update(float elapsed) {
	SpriteSheet::Row const &r = sheet.info.rows[row];
	time += elapsed;
	if (r.loop) {
		time = std::fmod(time, float(r.frames) / r.fps);
	} else if (time * r.fps >= float(r.frames)) {
		finished = true;
	}
}

uint32_t SpriteAnimator::frame() const {
	SpriteSheet::Row const &r = sheet.info.rows[row];
	uint32_t f = uint32_t(time * r.fps);
	return r.loop ? f % r.frames : std::min(f, r.frames - 1); // hold last frame for non loop
}
