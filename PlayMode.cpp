#include "PlayMode.hpp"
#include "SpriteSheet.hpp"

#include "LitColorTextureProgram.hpp"
#include "gl_compile_program.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "DrawLines.hpp"
#include "DrawSprites.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include "DummyEnemy.hpp"
#include "RatEnemy.hpp"
#include "GhostEnemy.hpp"

#include "Juice.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

struct RoomLayout
{
	glm::uvec2 size_px = glm::uvec2(0);
	std::vector<AABB> solids;
	glm::vec2 spawn_px = glm::vec2(0.0f);
	std::vector<glm::vec2> dummy_px;
	glm::vec2 exit_px = glm::vec2(0.0f);
	std::vector<AABB> hazards; 
	std::vector<glm::vec2> item_px;
	glm::vec2 keeper_px = glm::vec2(0.0f);
};

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
Load<SpriteSheet> tutorial_sheet(LoadTagDefault, []() -> SpriteSheet const *
								 { return load_static_image("sprites/tutorial-room.png", glm::uvec2(1440, 272)); });
Load<SpriteSheet> platform_sheet(LoadTagDefault, []() -> SpriteSheet const *
								 { return load_static_image("Platform.png", glm::uvec2(128, 86)); });

// dictates how big the room is in addition to roomscale in gen_params
Load<SpriteSheet> default_room_sheet(LoadTagDefault, []() -> SpriteSheet const *
									 { return load_static_image("Default Room Layout.png", glm::uvec2(512, 512)); });

Load<SpriteSheet> shop_room_sheet(LoadTagDefault, []() -> SpriteSheet const *
	{ return load_static_image("sprites/shop-room.png", glm::uvec2(512, 512)); });

static GLuint load_lighting_program() {
    std::ifstream vert_file(data_path("Raymarch.vert"));
    std::ifstream frag_file(data_path("Raymarch.frag"));

    std::stringstream vert_buffer;
    vert_buffer << vert_file.rdbuf();

    std::stringstream frag_buffer;
    frag_buffer << frag_file.rdbuf();

    GLuint program = gl_compile_program(
        vert_buffer.str(),
        frag_buffer.str()
    );

    return program;
}
// Control hints for the tutorial room
struct TutorialSign
{
	glm::vec2 pos;
	float size;
	char const *text;
};

static const std::vector<TutorialSign> tutorial_signs = {
	{glm::vec2(3.0f, 8.0f), 1.0f, "A / D : MOVE"},
	{glm::vec2(7.0f, 9.5f), 1.0f, "SPACE : JUMP"},
	{glm::vec2(13.0f, 12.0f), 1.0f, "HOLD SPACE TO JUMP HIGHER"},
	{glm::vec2(36.0f, 12.5f), 1.0f, "DASH (J) INTO ENEMIES TO HIT THEM"},
	{glm::vec2(50.0f, 8.2f), 1.0f, "WAVEDASH: S + A/D + J ON THE GROUND, THEN SPACE"},
	{glm::vec2(86.1f, 7.4f), 1.0f, "EXIT"},
};

static RoomLayout load_room_layout(std::string const &path)
{
	constexpr uint32_t Tile = 16;
	
	enum class TileKind {
		Empty, 
		Solid, 
		Hazard
	};

	RoomLayout layout;
	std::vector<glm::u8vec4> pixels;
	load_png(path, &layout.size_px, &pixels, LowerLeftOrigin);

	if (layout.size_px.x % Tile != 0 || layout.size_px.y % Tile != 0)
	{
		throw std::runtime_error("Room layout '" + path + "' is not a multiple of 16px.");
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
				}
				else if (c.r == 255 && c.g == 0 && c.b == 0)
				{ // red (255, 0, 0) = dummy spawn
					layout.dummy_px.push_back(feet);
				}
				else if (c.r == 255 && c.g == 255 && c.b == 0) { } // previously: items
				else if (c.r == 255 && c.g == 0 && c.b == 255) 
				{ // magenta (255, 0, 255) = shopkeeper / npc spawn
					layout.keeper_px = feet; 
				}
				else if (c.r == 0 && c.g == 0 && c.b == 255)
				{ // blue (0, 0, 255) = exit door
					layout.exit_px = feet;
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
	
	//aabb vertical merge
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
					AABB& a = aabbs[i];
					AABB& b = aabbs[j];

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

void PlayMode::buildShop(glm::vec2 origin_px)
{
	RoomLayout shop = load_room_layout(data_path("sprites/shop-layout.png"));
	if (shop.item_px.size() != 6)
	{
		throw std::runtime_error("shop-layout.png needs exactly 6 item markers."); // can be changed later 
	}
	addLevelSprite(*shop_room_sheet, origin_px, false, shop.solids);

	for(size_t i = 0; i < shop.item_px.size(); i++)
	{
		ShopSlot slot;
		slot.pos = (origin_px + shop.item_px[i]) / PPU; 
		slot.price = 5 + 5 * int(i); // placeholder price
		slot.icon = int(i); 
		shop_slots.push_back(slot);
	}
	shopkeeper_pos = (origin_px + shop.keeper_px + glm::vec2(8.0f, 0.0f)) / PPU; 
	shop_interior = AABB{glm::vec3((origin_px + glm::vec2(80.0f, 64.0f)) / PPU, 0.0f),
						glm::vec3((origin_px + glm::vec2(432.0f, 288.0f)) / PPU, 0.0f)};
}

void PlayMode::respawn()
{
	player.reset(spawn_pos);
	enemies.clear();
	ghost_spawned = false;
	if (use_tutorial)
	{
		for (glm::vec2 p : dummy_spawns)
		{
			enemies.emplace_back(std::make_unique<DummyEnemy>(p, *dummy_sheet));
		}
	}
	else
	{
		for (const auto &room : proc_gen.active_rooms)
		{
			if (room->kind == RoomKind::Shop) continue;
			const auto &room_template =
				get_room_templates(static_cast<int>(room->template_id));

			const glm::vec2 origin(room->world_x, room->world_y);

			for (const auto &spawn : room_template.enemies)
			{
				const glm::vec2 position =
					(origin + spawn.local_px) / PPU;

				switch (spawn.type)
				{
				case EnemyType::Dummy:
					enemies.emplace_back(std::make_unique<DummyEnemy>(position, *dummy_sheet));
					break;

				case EnemyType::Rat:
					enemies.emplace_back(std::make_unique<RatEnemy>(position, *rat_sheet));
					break;

				case EnemyType::ExplodingRat:
					break;
				}
			}
		}
	}

	for (auto &s : shop_slots)
	{
		s.sold = false;
	}

	cam_pos = spawn_pos;
	camera->transform->position = glm::vec3(cam_pos, 0.0f);

	player_dead = false;
}

PlayMode::PlayMode(bool tut) : use_tutorial(tut), player_anim(*main_character_sheet, "Idle")
{
	lighting_program = load_lighting_program();
	glGenVertexArrays(1, &lighting_vao);

	GameManager::Get().startFloor();

	colliders.reserve(128);

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

	Juice::Get().SetCamera(camera);

	if (use_tutorial)
	{
		RoomLayout tutorial = load_room_layout(data_path("sprites/tutorial-layout.png"));
		addLevelSprite(*tutorial_sheet, glm::vec2(0.0f), false, tutorial.solids);

		glm::vec2 exit_feet = tutorial.exit_px / PPU; 
		exit_zone = AABB{glm::vec3(exit_feet.x - 0.5f, exit_feet.y, 0.0f),
						 glm::vec3(exit_feet.x + 0.5f, exit_feet.y + 2.0f, 0.0f)};

		auto feet_to_center = [](glm::vec2 feet_px)
		{ return feet_px / PPU + glm::vec2(0.0f, 0.5f); };

		room_size = glm::vec2(tutorial.size_px) / PPU;
		spawn_pos = feet_to_center(tutorial.spawn_px);
		for (glm::vec2 d : tutorial.dummy_px)
		{
			dummy_spawns.push_back(feet_to_center(d));
		}
		for (AABB const &h : tutorial.hazards)
		{
			death_zones.push_back({h.min / PPU, h.max / PPU});
		}
		void_y = -2.0f;
	}
	else
	{ // room generation
		create_room_templates();
		create_templates_with_doors();
		proc_gen.params.room_scale = 512;
		proc_gen.generateLevel();
		proc_gen.addShopRoom();
		proc_gen.printGrid();

		spawn_pos = glm::vec2(160.0f, 153.0f) / PPU;

		glm::vec2 spawn_pos2 = glm::vec2(100.0f, 50.0f); 


		float lowest = 0.0f; 
		for (auto const &room : proc_gen.active_rooms)
		{
			lowest = std::min(lowest, room->world_y);
		}
		for (auto &room : proc_gen.active_rooms)
		{
			enemies.emplace_back(std::make_unique<DummyEnemy>(glm::vec2(room->world_x, room->world_y) + spawn_pos2, *dummy_sheet));

			if (room->kind == RoomKind::Shop)
			{
				buildShop(glm::vec2(room->world_x, room->world_y));
				// shop test spawn
				// spawn_pos = glm::vec2(room->world_x + 256.0f, room->world_y + 40.0f) / PPU; 
				continue;
			}
			std::vector<int> bucket = get_templates_with_doors(room->doors);
			if (!bucket.empty())
			{
				int pick = proc_gen.getRand(0, (int)(bucket.size() - 1));
				int template_id = bucket[pick];
				room->template_id = template_id;
				RoomTemplate t = get_room_templates(template_id);
				if (room->kind == RoomKind::Shop) t.enemies.clear(); 
				drawChunk(t, glm::vec2(room->world_x, room->world_y));

				if (room->kind == RoomKind::End)
				{
					// TODO: REPLACE WITH END ROOM LAYOUT 
					// Currently temporary bottom left-ish placement
					glm::vec2 o = glm::vec2(room->world_x + 176.0f, room->world_y + 48.0f) / PPU;
					exit_zone = AABB{glm::vec3(o.x - 0.5f, o.y, 0.0f), glm::vec3(o.x + 0.5f, o.y + 2.0f, 0.0f)};
				}
			}
		}
		void_y = lowest / PPU - 2.0f;
	}

	respawn();

	// Create low res framebuffer for rendering
	glGenTextures(1, &color_texture);
	glBindTexture(GL_TEXTURE_2D, color_texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, PixelWidth+1, PixelHeight+1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_texture, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GameManager::Get().setQuota((int)(enemies.size()), quota);
}

PlayMode::~PlayMode()
{
	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &color_texture);

	glDeleteVertexArrays(1, &lighting_vao);
    glDeleteProgram(lighting_program);
}

void update_input_dir()
{

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
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.downs += 1;
			right.pressed = true;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.downs += 1;
			up.pressed = true;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.downs += 1;
			down.pressed = true;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
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
		else if (evt.key.key == SDLK_R)
		{
			if (player_dead)
			{
				respawn();
			}
			return true;
		}
		else if (evt.key.key == SDLK_E) 
		{
			buy_pressed = true;
			return true;
		}
	}
	else if (evt.type == SDL_EVENT_KEY_UP)
	{
		if (evt.key.key == SDLK_A)
		{
			left.pressed = false;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_D)
		{
			right.pressed = false;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_W)
		{
			up.pressed = false;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
			return true;
		}
		else if (evt.key.key == SDLK_S)
		{
			down.pressed = false;
			input_dir = glm::vec2(float(right.pressed) - float(left.pressed),
								float(up.pressed) - float(down.pressed));
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
	total_time += elapsed;
	bool buy = buy_pressed; 
	buy_pressed = false; 
	anim_time += elapsed; 
	denied_timer = std::max(0.0f, denied_timer - elapsed);

	elapsed = Juice::Get().checkDeltaTime(elapsed);
	Juice::Get().update(elapsed);

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

	if (use_tutorial)
	{
		glm::vec2 half_view = glm::vec2(PixelWidth, PixelHeight) * 0.5f / PPU;
		cam_pos = glm::clamp(cam_pos, half_view, glm::max(room_size - half_view, half_view));
	}

	bool in_shop = shop_interior && checkCollision(player.collider.getWorldSpaceAABB(), *shop_interior);
	if (in_shop)
	{
		glm::vec2 center = glm::vec2(shop_interior->min + shop_interior->max) * 0.5f;
		cam_pos = glm::mix(glm::vec2(camera->transform->position), center, 1.0f - std::exp(-8.0f * elapsed));
	}

	{ // update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	if (!player_dead)
	{
		AABB player_box = player.collider.getWorldSpaceAABB();
		bool in_death_zone = std::any_of(death_zones.begin(), death_zones.end(),
							[&](AABB const& z) { return checkCollision(player_box, z); });
		
		if (player.health <= 0 || in_death_zone || player.transform->position.y < void_y)
		{
			player_dead = true;
		}
	}
	if (player_dead)
	{
		return;
	}

	player.move(input_dir, elapsed);

	player.updatePlayer(elapsed);

	runCollision(player, colliders);

	if (input_dir.x < 0.0f)
		facing_left = true;
	else if (input_dir.x > 0.0f)
		facing_left = false;

	hovered_slot = -1; 
	float best = 1.0f; 
	glm::vec2 p = glm::vec2(player.transform->position);
	for (size_t i = 0; i < shop_slots.size(); i++)
	{
		ShopSlot const &s = shop_slots[i];
		if (s.sold) continue; 
		float dx = std::abs(p.x - s.pos.x);
		if (dx < best && std::abs(p.y - s.pos.y) < 1.5f)
		{
			best = dx; 
			hovered_slot = int(i);
		}
	}

	if (buy && hovered_slot >= 0)
	{
		ShopSlot &s = shop_slots[hovered_slot];
		if (GameManager::Get().spend(s.price))
		{
			s.sold = true;  // TODO : give item to player
		}
		else
		{
			denied_timer = 0.6f;
			Juice::Get().addTrauma(0.3f, 0.15f, glm::vec2(1.0f, 0.0f));
		}
	}

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

			if (player.is_attacking && std::find(player.hit_enemies.begin(), player.hit_enemies.end(), enemy.get()) == player.hit_enemies.end())
			{
				enemy->takeDamage(player.calcualteDamage(), player.style_meter.getCoinMultiplier());
				player.dashBounce();
				player.hit_enemies.push_back(enemy.get());
				break;
			}
			else if (player.takeDamage(enemy->contact_damage, glm::vec2(enemy->transform.position)))
			{
				break;
			}
		}
	}

	if (exit_zone && checkCollision(player.collider.getWorldSpaceAABB(), *exit_zone))
	{
		if (use_tutorial)
		{
			GameManager::Get().tutorial_done = true;
			Mode::set_current(std::make_shared<PlayMode>(false));
			return;
		}
		else if (GameManager::Get().quotaMet())
		{
			Mode::set_current(std::make_shared<PlayMode>(false)); // next floor
			return; 
		}
	}

	if ((total_time > 10.0f * proc_gen.active_rooms.size()) && !ghost_spawned && !use_tutorial)
	{
		ghost_spawned = true;
		enemies.emplace_back(std::make_unique<GhostEnemy>(player.transform->position + glm::vec3(25.0f, 25.0f, 0.0f), *dummy_sheet));
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

	glm::vec2 cam_pos_ppu = glm::floor((glm::vec2(camera->transform->position) + camera->shake_extra_pos) * PPU);

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
		if (!use_tutorial)
		{
			for (const auto &room : proc_gen.active_rooms)
			{
				if (room->kind == RoomKind::Shop) continue;

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

	{
		// Compute inputs to raymarching shader
		Light player_light{
			.position = glm::vec2(player.transform->position),
			.color = glm::vec3(1.0f, 0.75f, 0.45f),
			.radius = 12.0f,
			.intensity = 1.4f,
			.do_march = true,
		};

		glm::vec2 offset = screen_center - cam_pos_ppu;

		glm::vec2 light_px = player_light.position * PPU + offset;
		float radius_px = player_light.radius * PPU;

		std::vector<glm::vec4> light_blockers;
		
		for (const Collider &collider : colliders)
		{
			if (collider.layer != CollisionLayer::Level) continue;
			AABB box = collider.getWorldSpaceAABB();

			glm::vec2 box_center = (glm::vec2(box.min) + glm::vec2(box.max)) * 0.5f;
			
			float delta = glm::distance(box_center, glm::vec2(player_light.position));

			if (delta > (player_light.radius * 1.2f) + glm::length(glm::vec2(box.max) - glm::vec2(box.min))/2.0f) {
				continue;
			}

			glm::vec2 min_px = glm::vec2(box.min) * PPU + offset;
			glm::vec2 max_px = glm::vec2(box.max) * PPU + offset;

			light_blockers.emplace_back(min_px, max_px);
		}

		glUseProgram(lighting_program);

		glUniform1i(
			glGetUniformLocation(lighting_program, "BLOCKER_COUNT"),
			static_cast<GLint>(light_blockers.size())
		);

		glUniform4fv(
			glGetUniformLocation(lighting_program, "BLOCKER_BBS[0]"),
			static_cast<GLsizei>(light_blockers.size()),
			glm::value_ptr(light_blockers.front())
		);

		glUniform1i(glGetUniformLocation(lighting_program, "LIGHT_COUNT"), 1);

		glUniform2fv(
			glGetUniformLocation(lighting_program, "LIGHTS[0].position"),
			1, glm::value_ptr(light_px)
		);

		glUniform3fv(
			glGetUniformLocation(lighting_program, "LIGHTS[0].color"),
			1, glm::value_ptr(player_light.color)
		);

		glUniform1f(
			glGetUniformLocation(lighting_program, "LIGHTS[0].radius"),
			radius_px
		);

		glUniform1f(
			glGetUniformLocation(lighting_program, "LIGHTS[0].intensity"),
			player_light.intensity
		);

		glUniform1i(
			glGetUniformLocation(lighting_program, "LIGHTS[0].do_march"),
			player_light.do_march ? 1 : 0
		);

		glDisable(GL_DEPTH_TEST);
		glEnable(GL_FRAMEBUFFER_SRGB);
		glEnable(GL_BLEND);

		glBlendEquation(GL_FUNC_ADD);

		glBlendFuncSeparate(
			GL_DST_COLOR, GL_ZERO,
			GL_ZERO, GL_ONE
		);

		glBindVertexArray(lighting_vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);
		glUseProgram(0);

		glDisable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


	}

	{
		DrawLines lines(world_to_clip);

		if (use_tutorial)
		{
			for (TutorialSign const &sign : tutorial_signs)
			{
				lines.draw_text(sign.text,
								glm::vec3(sign.pos, 0.0f),
								glm::vec3(sign.size, 0.0f, 0.0f), glm::vec3(0.0f, sign.size, 0.0f),
								glm::u8vec4(255, 225, 160, 255));
			}
		}

		AABB world_player_aabb = player.collider.getWorldSpaceAABB();
		glm::vec3 player_center = (world_player_aabb.min + world_player_aabb.max) * 0.5f;
		glm::vec3 player_extents = (world_player_aabb.max - world_player_aabb.min) * 0.5f;

		glm::mat4x3 player_draw_transform = glm::translate(glm::mat4(1.0f), player_center) * glm::scale(glm::mat4(1.0f), player_extents);

		//lines.draw_box(player_draw_transform, glm::u8vec4(0, 255, 0, 255));

		for (auto const &enemy : enemies)
		{
			if (enemy->state == EnemyState::Dead)
				continue;

			AABB world_enemy_aabb = enemy->collider.getWorldSpaceAABB();

			glm::vec3 enemy_center = (world_enemy_aabb.min + world_enemy_aabb.max) * 0.5f;
			glm::vec3 enemy_extents = (world_enemy_aabb.max - world_enemy_aabb.min) * 0.5f;

			glm::mat4x3 enemy_draw_transform = glm::translate(glm::mat4(1.0f), enemy_center) * glm::scale(glm::mat4(1.0f), enemy_extents);

			lines.draw_box(enemy_draw_transform, glm::u8vec4(255, 180, 0, 255));

			lines.draw_text(
				std::to_string(enemy->health),
				glm::vec3(world_enemy_aabb.min.x, world_enemy_aabb.max.y + 0.2f, 0.0f),
				glm::vec3(0.3f, 0.0f, 0.0f),
				glm::vec3(0.0f, 0.3f, 0.0f),
				glm::u8vec4(255, 255, 255, 255));
		}

		// TODO: replace with actual shop icons rather than DrawLines code
		for (size_t i = 0; i < shop_slots.size(); i++)
		{
			ShopSlot const &s = shop_slots[i];
			if (s.sold) continue;

			float bob = 0.15f * std::sin(anim_time * 3.0f + float(i));
			glm::vec3 center = glm::vec3(s.pos + glm::vec2(0.0f, 0.5f + bob), 0.0f);
			lines.draw_box(glm::translate(glm::mat4(1.0f), center) *
						glm::scale(glm::mat4(1.0f), glm::vec3(0.4f)),
						glm::u8vec4(255, 220, 120, 255));   // placeholder icon

			std::string price = std::to_string(s.price);
			bool affordable = GameManager::Get().getMoney() >= s.price;
			float size = 0.5f;
			lines.draw_text(price,
				glm::vec3(s.pos.x - 0.2f * size * float(price.size()), s.pos.y - 0.6f, 0.0f),
				glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f, size, 0.0f),
				affordable ? glm::u8vec4(255, 230, 120, 255) : glm::u8vec4(220, 80, 80, 255));

			if (int(i) == hovered_slot)
			{
				lines.draw_text("E: BUY",
					glm::vec3(s.pos.x - 0.6f, s.pos.y + 1.4f, 0.0f),
					glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f, size, 0.0f),
					glm::u8vec4(255, 255, 255, 255));
			}
		}

		if (denied_timer > 0.0f)
		{
			glm::vec2 pp = glm::vec2(player.transform->position);
			lines.draw_text("NOT ENOUGH MONEY",
				glm::vec3(pp.x - 1.6f, pp.y + 1.2f, 0.0f),
				glm::vec3(0.5f, 0.0f, 0.0f), glm::vec3(0.0f, 0.5f, 0.0f),
				glm::u8vec4(220, 80, 80, 255));
		}

		auto [enemies_killed, quota_enemies] = GameManager::Get().getEnemiesKilledStats();
		lines.draw_text(std::to_string(enemies_killed) + " / " + std::to_string(quota_enemies),
							glm::vec3(cam_pos.x - 10.0f, cam_pos.y + 6.5f, 0.0f),
							glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
							glm::u8vec4(255, 255, 255, 255));

		lines.draw_text(std::to_string(player.style_meter.getCoinMultiplier()) + "x  " + std::to_string(player.style_meter.style_cur),
					glm::vec3(cam_pos.x - 10.0f, cam_pos.y - 6.5f, 0.0f),
					glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
					glm::u8vec4(255, 255, 255, 255));


		lines.draw_text(std::to_string(GameManager::Get().getMoney()) + " MONEY",
							glm::vec3(cam_pos.x + 10.0f, cam_pos.y + 6.5f, 0.0f),
							glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
							glm::u8vec4(255, 255, 255, 255));

		// TODO: REPLACE WITH END ROOM LAYOUT 
		// temporary placeholder marker
		if (exit_zone && !use_tutorial)
		{
			bool open = GameManager::Get().quotaMet();
			glm::vec3 c = (exit_zone->min + exit_zone->max) * 0.5f;
			glm::vec3 half = (exit_zone->max - exit_zone->min) * 0.5f;
			glm::u8vec4 col = open ? glm::u8vec4(120, 255, 140, 255) : glm::u8vec4(220, 80, 80, 255);

			lines.draw_box(glm::translate(glm::mat4(1.0f), c) * glm::scale(glm::mat4(1.0f), half), col);

			auto [killed, needed] = GameManager::Get().getEnemiesKilledStats();
			std::string label = open ? "EXIT" : "KILL " + std::to_string(needed - killed) + " MORE";
			float size = 0.5f;
			lines.draw_text(label,
				glm::vec3(c.x - 0.2f * size * float(label.size()), exit_zone->max.y + 0.3f, 0.0f),
				glm::vec3(size, 0.0f, 0.0f), glm::vec3(0.0f, size, 0.0f), col);
		}


		if (player_dead)
		{
			lines.draw_text("PRESS R TO RESTART",
							glm::vec3(cam_pos.x - 3.0f, cam_pos.y + 2.0f, 0.0f),
							glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
							glm::u8vec4(255, 255, 255, 255));
		}
	}

	float scale_x = (float)drawable_size.x / PixelWidth;
	float scale_y = (float)drawable_size.y / PixelHeight;

	glm::vec2 exact_px = (glm::vec2(camera->transform->position) + camera->shake_extra_pos) * PPU;
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
}