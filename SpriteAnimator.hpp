#pragma once

#include "SpriteSheet.hpp"
#include <string>

//Playback state for one character
struct SpriteAnimator {
	SpriteAnimator(SpriteSheet const &sheet, std::string const &start_row);

	//switch animation; only restarts if it's a different row (or restart is true):
	void play(std::string const &name, bool restart = false);
	void update(float elapsed);
	uint32_t frame() const;

	SpriteSheet const &sheet;
	uint32_t row = 0;
	float time = 0.0f;
	bool finished = false;
};
