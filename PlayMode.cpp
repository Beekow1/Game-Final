#include "PlayMode.hpp"
#include "SpriteSheet.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "DrawSprites.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include "DummyEnemy.hpp"
#include "RatEnemy.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

Load<Sound::Sample> dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const *
									   { return new Sound::Sample(data_path("dusty-floor.opus")); });

Load<Sound::Sample> honk_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("honk.wav")); });

Load<SpriteSheet> main_character_sheet(LoadTagDefault, []() -> SpriteSheet const *
									   { return new SpriteSheet(data_path("sprites/main-character.png"), SpriteSheet::Info{
																											 .cell_size = glm::uvec2(48, 24),
																											 .rows = {
																												 {"Walk", 8, 8.0f, true},
																												 {"Idle", 10, 8.0f, true},
																												 {"Jump", 3, 8.0f, false},
																												 {"Falling", 4, 8.0f, true},
																												 {"Walk2Idle", 1, 8.0f, false}, // transition, then switch to Idle
																											 },
																										 }); });

Load<SpriteSheet> dummy_sheet(LoadTagDefault, []() -> SpriteSheet const *
							  { return new SpriteSheet(data_path("sprites/Dummy.png"), SpriteSheet::Info{
																						   .cell_size = glm::uvec2(24, 48),
																						   .rows = {
																							   {"Idle", 1, 10.0f, true},
																							   {"Hurt", 5, 10.0f, false},
																						   },
																					   }); });

Load<SpriteSheet> blocker_sheet(LoadTagDefault, []() -> SpriteSheet const *
								{ return new SpriteSheet(data_path("Default Room Blocker-Sheet.png"), SpriteSheet::Info{
																										  .cell_size = glm::uvec2(512, 512),
																										  .rows = {
																											  {"Left", 1, 1.0f, false},
																											  {"Down", 1, 1.0f, false},
																											  {"Right", 1, 1.0f, false},
																											  {"Up", 1, 1.0f, false},
																										  },
																									  }); });

Load<SpriteSheet> rat_sheet(LoadTagDefault, []() -> SpriteSheet const *
							{ return new SpriteSheet(data_path("Def Mouse-Sheet.png"), SpriteSheet::Info{
																						   .cell_size = glm::uvec2(48, 24),
																						   .rows = {
																							   {"Idle", 4, 10.0f, true},
																						   },
																					   }); });

Load<SpriteSheet> exploding_rat_sheet(LoadTagDefault, []() -> SpriteSheet const *
									  { return new SpriteSheet(data_path("Explo Mouse-Sheet.png"), SpriteSheet::Info{
																									   .cell_size = glm::uvec2(48, 24),
																									   .rows = {
																										   {"Idle", 4, 10.0f, true},
																									   },
																								   }); });

Load<SpriteSheet> heart_sheet(LoadTagDefault, []() -> SpriteSheet const *
							  { return new SpriteSheet(data_path("Heart.png"), SpriteSheet::Info{
																				   .cell_size = glm::uvec2(16, 16), // lets hope this is enough!
																				   .rows = {
																					   {"Hearts", 2, 1.0f, false},
																				   },
																			   }); });

static SpriteSheet *load_static_image(std::string const &file_path, glm::uvec2 size)
{
	return new SpriteSheet(data_path(file_path), SpriteSheet::Info{
													 .cell_size = size,
													 .rows = {
														 {"still", 1, 1.0f, true}}});
}

Load<SpriteSheet> platform_sheet(LoadTagDefault, []() -> SpriteSheet const *
								 { return load_static_image("Platform.png", glm::uvec2(128, 86)); });

Load<SpriteSheet> decoration_one_sheet(LoadTagDefault, []() -> SpriteSheet const *
									   { return load_static_image("Platform Decoration 1.png", glm::uvec2(128, 86)); });

Load<SpriteSheet> decoration_two_sheet(LoadTagDefault, []() -> SpriteSheet const *
									   { return load_static_image("Platform Decoration 2.png", glm::uvec2(128, 86)); });

Load<SpriteSheet> barrels_sheet(LoadTagDefault, []() -> SpriteSheet const *
								{ return load_static_image("Drink Barrels.png", glm::uvec2(112, 32)); });

// dictates how big the room is in addition to roomscale in gen_params
Load<SpriteSheet> default_room_sheet(LoadTagDefault, []() -> SpriteSheet const *
									 { return load_static_image("Default Room Layout.png", glm::uvec2(512, 512)); });

Load<SpriteSheet> shelf_sheet(LoadTagDefault, []() -> SpriteSheet const *
							  { return load_static_image("Bottle Shelf.png", glm::uvec2(64, 80)); });

PlayMode::PlayMode() : player_anim(*main_character_sheet, "Idle")
{
	create_room_templates();
	create_templates_with_doors();
	colliders.reserve(128);
	proc_gen.params.room_scale = 512;
	proc_gen.generateLevel();
	proc_gen.printGrid();

	scene.transforms.emplace_back();
	Scene::Transform *camera_transform = &scene.transforms.back();
	camera_transform->position = glm::vec3(0.0f);
	scene.cameras.emplace_back(camera_transform);
	camera = &scene.cameras.back();

	scene.transforms.emplace_back();
	Scene::Transform *player_transform = &scene.transforms.back();

	player_transform->position = glm::vec3(160.0f, 153.0f, 0.0f) / PPU;
	player.transform = player_transform;
	player.collider.transform = player_transform;

	player.collider.layer = CollisionLayer::Player;
	player.collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Level);

	player.collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, -0.5f);
	player.collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.5f);

	colliders.emplace_back(player.collider);

	AABB platform_bb;
	platform_bb.min = glm::vec3(0.0f, 67.0f, 0.0f);
	platform_bb.max = glm::vec3(128.0f, 86.0f, 0.0f);

	glm::vec3 down_min = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 down_max = glm::vec3(gen_params.room_scale, 1.0f, 0.0f);

	glm::vec3 up_min = glm::vec3(0.0f, 511.0f, 0.0f);
	glm::vec3 up_max = glm::vec3(gen_params.room_scale, gen_params.room_scale, 0.0f);

	glm::vec3 left_min = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 left_max = glm::vec3(1.0f, gen_params.room_scale, 0.0f);

	glm::vec3 right_min = glm::vec3(511.0f, 0.0f, 0.0f);
	glm::vec3 right_max = glm::vec3(gen_params.room_scale, gen_params.room_scale, 0.0f);

	for (size_t i = 0; i < proc_gen.active_rooms.size(); i++)
	{
		std::shared_ptr<Room> room = proc_gen.active_rooms[i];
		glm::vec3 pos = glm::vec3(room->world_x, room->world_y, 0.0f);

		std::vector<AABB> level_bbs;
		level_bbs.reserve(4);

		if (!(room->doors & Doors::Left))
		{
			level_bbs.push_back({left_min, left_max});
		}
		if (!(room->doors & Doors::Right))
		{
			level_bbs.push_back({right_min, right_max});
		}
		if (!(room->doors & Doors::Up))
		{
			level_bbs.push_back({up_min, up_max});
		}
		if (!(room->doors & Doors::Down))
		{
			level_bbs.push_back({down_min, down_max});
		}

		std::vector<int> bucket = get_templates_with_doors(room->doors);
		if (!bucket.empty())
		{
			int pick = proc_gen.getRand(0, (int)(bucket.size() - 1));
			int template_id = bucket[pick];
			room->template_id = template_id;
			drawChunk(get_room_templates(template_id), glm::vec2(room->world_x, room->world_y));
		}
	}

	// Create low res framebuffer for rendering
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, PixelWidth, PixelHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PlayMode::~PlayMode()
{
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &color_texture);
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (evt.key.repeat)
			return true;
		if (evt.key.key == SDLK_ESCAPE)
		{
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		}
		else if (evt.key.key == SDLK_A)
		{
			left.downs += 1;
			left.pressed = true;
			input_dir += glm::vec2(-1.0f, 0.0f);
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.downs += 1;
			right.pressed = true;
			input_dir += glm::vec2(1.0f, 0.0f);
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.downs += 1;
			up.pressed = true;
			input_dir += glm::vec2(0.0f, 1.0f);
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.downs += 1;
			down.pressed = true;
			input_dir += glm::vec2(0.0f, -1.0f);
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			J.downs += 1;
			J.pressed = true;
			player.dash(input_dir);
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			player.jump_not_wasted = true;
			player.jump_buffer_timer = player.jump_buffer_duration;
			// if (honk_oneshot) honk_oneshot->stop();
			// honk_oneshot = Sound::play_3D(*honk_sample, 0.3f, glm::vec3(4.6f, -7.8f, 6.9f)); //hardcoded position of front of car, from blender
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A)
		{
			left.pressed = false;
			input_dir -= glm::vec2(-1.0f, 0.0f);
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.pressed = false;
			input_dir -= glm::vec2(1.0f, 0.0f);
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.pressed = false;
			input_dir -= glm::vec2(0.0f, 1.0f);
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.pressed = false;
			input_dir -= glm::vec2(0.0f, -1.0f);
			return true;
		}
		else if (evt.key.key == SDLK_J)
		{
			J.pressed = false;
			return true;
		}
		else if (evt.key.key == SDLK_SPACE)
		{
			player.jump_not_wasted = false;
			player.attemptJumpCut();
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
	{
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false)
		{
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_MOUSE_MOTION)
	{
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	player.jump_buffer_timer -= elapsed;

	if (!player.is_grounded)
	{
		player.coyote_timer -= elapsed;
	}

	float t = elapsed / cam_smoothing;
	t = glm::clamp(t, 0.0f, 1.0f);

	if (input_dir.x > 0.0f)
	{
		look_ahead_dir = input_dir.x;
	}

	target_cam_pos = player.transform->position + glm::vec3((look_ahead_dir * look_ahead), 0.0f, 0.0f);

	glm::vec2 offset = glm::vec2(player.transform->position) - glm::vec2(camera->transform->position);

	float excess_x = glm::max(0.0f, glm::abs(offset.x) - (dead_zone_x / 2.0f));
	float excess_y = glm::max(0.0f, glm::abs(offset.y) - (dead_zone_y / 2.0f));
	float total_excess_distance = glm::length(glm::vec2(excess_x, excess_y));
	float dynamic_speed = total_excess_distance * pull_strength;

	float smoothT = 1.0f - glm::exp(-dynamic_speed * elapsed);

	cam_pos = glm::mix(glm::vec2(camera->transform->position), target_cam_pos, smoothT);

	{ // update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	player.move(input_dir, elapsed);

	player.updatePlayer(elapsed);

	runCollision(player, colliders);

	if (input_dir.x < 0.0f)
		facing_left = true;
	else if (input_dir.x > 0.0f)
		facing_left = false;

	// Player Animation Update

	if (!player.is_grounded)
	{
		player_anim.play(player.velocity.y > 0.0f ? "Jump" : "Falling");
	}
	else if (std::abs(player.velocity.x) > 0.5f)
	{
		player_anim.play("Walk");
	}
	else if (player_anim.row == main_character_sheet->row_index("Walk"))
	{
		player_anim.play("Walk2Idle");
	}
	else if (player_anim.row != main_character_sheet->row_index("Walk2Idle") || player_anim.finished)
	{
		player_anim.play("Idle");
	}
	player_anim.update(elapsed);

	for (auto const &enemy : enemies)
	{
		enemy->update(elapsed, colliders);
	}

	// replaced enemy erase statement and moved outside of loop
	// because it would cause a bug when the dummy died
	std::erase_if(enemies, [](auto const &e)
				  { return e->state == EnemyState::Dead; });

	if (player.health > 0)
	{
		const AABB player_bounds = player.collider.getWorldSpaceAABB();

		for (auto const &enemy : enemies)
		{
			if (enemy->state == EnemyState::Dead)
			{
				continue;
			}

			if (!checkCollision(player_bounds, enemy->collider.getWorldSpaceAABB()))
			{
				continue;
			}

			if (player.is_attacking)
			{
				enemy->takeDamage(player.dash_damage);
				player.dashBounce();

				break;
			}
			else if (player.takeDamage(enemy->contact_damage, glm::vec2(enemy->transform.position)))
			{
				break;
			}
		}
	}

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	const int fbo_w = PixelWidth + 1;
	const int fbo_h = PixelHeight + 1;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, fbo_w, fbo_h);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glClearColor(0.02518f, 0.01298f, 0.03071f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const glm::mat4 proj = glm::ortho(0.0f, (float)fbo_w, 0.0f, (float)fbo_h);

	camera->transform->position = glm::vec3(cam_pos, 0.0f);

	glm::vec2 cam_pos_ppu = glm::floor(glm::vec2(camera->transform->position) * PPU);

	glm::vec2 screen_center = glm::vec2(PixelWidth / 2.0f, PixelHeight / 2.0f);
	glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(screen_center - cam_pos_ppu, 0.0f));

	const glm::mat4 view_proj = proj * view;

	const glm::mat4 world_to_clip = view_proj *
									glm::scale(glm::mat4(1.0f), glm::vec3(PPU, PPU, 1.0f));

	{
		DrawSprites blockers(*blocker_sheet, view_proj);

		uint32_t left_row = blocker_sheet->row_index("Left");
		uint32_t down_row = blocker_sheet->row_index("Down");
		uint32_t right_row = blocker_sheet->row_index("Right");
		uint32_t up_row = blocker_sheet->row_index("Up");

		for (StaticSprite const &sprite : level_sprites)
		{
			if (sprite.is_fg)
				continue;

			DrawSprites sprites(*sprite.sheet, view_proj);
			sprites.draw(0, 0, glm::vec2(sprite.transform->position) * PPU + glm::vec2(static_cast<float>(sprite.sheet->info.cell_size.x / 2u), 0.0f));
		}

		for (StaticSprite const &sprite : level_sprites)
		{
			if (!sprite.is_fg)
				continue;

			DrawSprites sprites(*sprite.sheet, view_proj);
			sprites.draw(0, 0, glm::vec2(sprite.transform->position) * PPU + glm::vec2(static_cast<float>(sprite.sheet->info.cell_size.x / 2u), 0.0f));
		}

		for (const auto &room : proc_gen.active_rooms)
		{

			if (!(room->doors & Doors::Left))
				blockers.draw(left_row, 0, glm::vec2(room->world_x + 256.0f, room->world_y));

			if (!(room->doors & Doors::Down))
				blockers.draw(down_row, 0, glm::vec2(room->world_x + 256.0f, room->world_y));

			if (!(room->doors & Doors::Right))
				blockers.draw(right_row, 0, glm::vec2(room->world_x + 256.0f, room->world_y));

			if (!(room->doors & Doors::Up))
				blockers.draw(up_row, 0, glm::vec2(room->world_x + 256.0f, room->world_y));
		}
	}
	{ // player
		AABB box = player.collider.getWorldSpaceAABB();
		glm::vec2 feet = glm::vec2(player.transform->position.x, box.min.y) * PPU;
		DrawSprites sprites(*main_character_sheet, view_proj);
		sprites.draw(player_anim.row, player_anim.frame(), feet, facing_left);
	}

	{ // enemies
		for (auto const &e : enemies)
		{
			AABB box = e->collider.getWorldSpaceAABB();
			glm::vec2 feet = glm::vec2(e->transform.position.x, box.min.y) * PPU;
			DrawSprites sprites(e->anim.sheet, view_proj);
			sprites.draw(e->anim.row, e->anim.frame(), feet, e->facing_left);
		}
	}

	float scale_x = (float)drawable_size.x / PixelWidth;
	float scale_y = (float)drawable_size.y / PixelHeight;

	glm::vec2 exact_px = glm::vec2(camera->transform->position) * PPU;
	glm::vec2 frac = exact_px - cam_pos_ppu;

	GLint offset_x = static_cast<GLint>(std::floor(frac.x * scale_x));
	GLint offset_y = static_cast<GLint>(std::floor(frac.y * scale_y));

	GLint dst_w = (GLint)std::lround(fbo_w * scale_x);
	GLint dst_h = (GLint)std::lround(fbo_h * scale_y);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glViewport(0, 0, drawable_size.x, drawable_size.y);

	glDisable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, fbo_w, fbo_h,
					  -offset_x, -offset_y, dst_w - offset_x, dst_h - offset_y,
					  GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_FRAMEBUFFER_SRGB);

	{ // Health HUD: fixed screen coordinates.
		const glm::mat4 ui_proj = glm::ortho(0.0f, static_cast<float>(PixelWidth), 0.0f, static_cast<float>(PixelHeight));

		constexpr float gap = 2.0f;
		constexpr float margin = 8.0f;

		float x = margin + 8.0f /* half size to anchor at center*/;
		float bot = static_cast<float>(PixelHeight) - margin - 16.0f;
		DrawSprites hearts(*heart_sheet, ui_proj);

		for (int i = player.max_health - 1; i >= 0; --i)
		{
			hearts.draw(0, (player.max_health - i - 1 < player.health) ? 0u : 1u, glm::vec2(x, bot));
			x += 16.0f + gap;
		}
	}

	GL_ERRORS();
}

Scene::Transform *PlayMode::addLevelSprite(SpriteSheet const &sheet, glm::vec2 bl, bool is_fg, std::vector<AABB> const &obj_AABBs)
{
	scene.transforms.emplace_back();
	Scene::Transform *sprite_transform = &scene.transforms.back();

	sprite_transform->position = glm::vec3(bl / PPU, 0.0f);

	level_sprites.push_back(StaticSprite{&sheet, sprite_transform, is_fg});

	if (!obj_AABBs.empty())
	{
		for (size_t i = 0; i < obj_AABBs.size(); i++)
		{
			Collider collider;
			collider.transform = sprite_transform;

			collider.local_aabb.min = obj_AABBs[i].min / PPU;
			collider.local_aabb.max = obj_AABBs[i].max / PPU;

			collider.layer = CollisionLayer::Level;
			collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Player) | static_cast<std::uint32_t>(CollisionLayer::Enemy);

			colliders.push_back(collider);
		}
	}

	return sprite_transform;
}

void PlayMode::drawChunk(RoomTemplate template_room, glm::vec2 room_origin)
{
	{
		std::vector<AABB> room_colliders = template_room.colliders;

		if (!(template_room.doors & Doors::Left))

			room_colliders.push_back({glm::vec3(0.0f, 64.0f, 0.0f), glm::vec3(16.0f, 128.0f, 0.0f)});

		if (!(template_room.doors & Doors::Right))

			room_colliders.push_back({glm::vec3(496.0f, 64.0f, 0.0f), glm::vec3(512.0f, 128.0f, 0.0f)});

		if (!(template_room.doors & Doors::Down))

			room_colliders.push_back({glm::vec3(224.0f, 0.0f, 0.0f), glm::vec3(288.0f, 48.0f, 0.0f)});

		if (!(template_room.doors & Doors::Up))

			room_colliders.push_back({glm::vec3(240.0f, 496.0f, 0.0f), glm::vec3(272.0f, 512.0f, 0.0f)});

		addLevelSprite(*default_room_sheet, room_origin, false, room_colliders);
	}

	for (auto &enemy : template_room.enemies)
	{
		const glm::vec2 enemy_position = (room_origin + enemy.local_px) / PPU;
		switch (enemy.type)
		{
		case EnemyType::Dummy:
			enemies.emplace_back(std::make_unique<DummyEnemy>(enemy_position, *dummy_sheet));
			break;

		case EnemyType::Rat:
			enemies.emplace_back(std::make_unique<RatEnemy>(enemy_position, *rat_sheet));
			break;

		case EnemyType::ExplodingRat:
			break;
		}
	}
}