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

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const *
								{
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret; });

Load<Scene> hexapod_scene(LoadTagDefault, []() -> Scene const *
						  { return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
											 {
												 Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

												 scene.drawables.emplace_back(transform);
												 Scene::Drawable &drawable = scene.drawables.back();

												 drawable.pipeline = lit_color_texture_program_pipeline;

												 drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
												 drawable.pipeline.type = mesh.type;
												 drawable.pipeline.start = mesh.start;
												 drawable.pipeline.count = mesh.count; }); });

Load<Sound::Sample> dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const *
									   { return new Sound::Sample(data_path("dusty-floor.opus")); });

Load<Sound::Sample> honk_sample(LoadTagDefault, []() -> Sound::Sample const *
								{ return new Sound::Sample(data_path("honk.wav")); });

Load<SpriteSheet> test_character_sheet(LoadTagDefault, []() -> SpriteSheet const *
									   { return new SpriteSheet(data_path("sprites/test-character.png"), SpriteSheet::Info{
																											 .cell_size = glm::uvec2(16, 24),
																											 .rows = {
																												 {"idle", 2, 4.0f, true},
																												 {"walk", 4, 10.0f, true},
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

Load<SpriteSheet> shelf_sheet(LoadTagDefault, []() -> SpriteSheet const *
							  { return load_static_image("Bottle Shelf.png", glm::uvec2(64, 80)); });

PlayMode::PlayMode() : scene(*hexapod_scene),
					   idle_anim(*test_character_sheet, "idle"),
					   walk_anim(*test_character_sheet, "walk")
{

	colliders.reserve(10);

	scene.transforms.emplace_back();
	Scene::Transform *player_transform = &scene.transforms.back();

	player_transform->position = glm::vec3(160.0f, 158.0f, 0.0f) / PPU;
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

	addLevelSprite(*platform_sheet, glm::vec2(96.0f, 48.0f), true, platform_bb);
	addLevelSprite(*platform_sheet, glm::vec2(256.0f, 48.0f), true, platform_bb);

	addLevelSprite(*decoration_one_sheet, glm::vec2(96.0f, 48.0f), false);
	addLevelSprite(*decoration_two_sheet, glm::vec2(256.0f, 48.0f), false);
	addLevelSprite(*shelf_sheet, glm::vec2(122.0f, 134.0f), false);
	addLevelSprite(*barrels_sheet, glm::vec2(265.0f, 134.0f), false);

	enemies.emplace_back(std::make_unique<DummyEnemy>(glm::vec2(320.0f, 166.0f) / PPU));

	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms)
	{
		if (transform.name == "Hip.FL")
			hip = &transform;
		else if (transform.name == "UpperLeg.FL")
			upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL")
			lower_leg = &transform;
	}
	if (hip == nullptr)
		throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr)
		throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr)
		throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// start music loop playing:
	//  (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);

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
		else if (evt.key.key == SDLK_K)
		{
			if (!evt.key.repeat)
			{
				for (auto const &enemy : enemies)
				{
					if (enemy->state != EnemyState::Dead)
					{
						enemy->takeDamage(10);
						break;
					}
				}
			}

			return true;
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
			// SDL_SetWindowRelativeMouseMode(Mode::window, true);
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

	// slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble);

	player.jump_buffer_timer -= elapsed;

	if (!player.is_grounded)
	{
		player.coyote_timer -= elapsed;
	}

	// move sound to follow leg tip position:
	leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	{ // update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	player.move(input_dir, elapsed);

	player.updatePlayer(elapsed);

	runCollision(player, colliders);

	for (auto const &enemy : enemies)
	{
		enemy->update(elapsed, colliders);

		if (enemy->state == EnemyState::Dead)
		{
			enemies.erase(std::remove(enemies.begin(), enemies.end(), enemy), enemies.end());
		}
	}

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

			if (player.is_dashing)
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

	idle_anim.update(elapsed);
	walk_anim.update(elapsed);
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, PixelWidth, PixelHeight);

	glEnable(GL_FRAMEBUFFER_SRGB);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glClearColor(0.00518f, 0.00605f, 0.01096f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const glm::mat4 proj = glm::ortho(0.0f, static_cast<float>(PixelWidth), 0.0f, static_cast<float>(PixelHeight));
	const glm::mat4 world_to_clip = proj *
									glm::scale(glm::mat4(1.0f), glm::vec3(PPU, PPU, 1.0f));

	for (StaticSprite const &sprite : level_sprites)
	{
		if (sprite.is_fg)
			continue;

		DrawSprites sprites(*sprite.sheet, proj);
		sprites.draw(0, 0, glm::vec2(sprite.transform->position) * PPU + glm::vec2(static_cast<float>(sprite.sheet->info.cell_size.x / 2u), 0.0f));
	}

	for (StaticSprite const &sprite : level_sprites)
	{
		if (!sprite.is_fg)
			continue;

		DrawSprites sprites(*sprite.sheet, proj);
		sprites.draw(0, 0, glm::vec2(sprite.transform->position) * PPU + glm::vec2(static_cast<float>(sprite.sheet->info.cell_size.x / 2u), 0.0f));
	}

	//	{ // sprites
	//		glm::mat4 world_to_clip = glm::ortho(lo.x, lo.x + view.x, lo.y, lo.y + view.y);
	//		DrawSprites sprites(*test_character_sheet, world_to_clip);
	//		sprites.draw(idle_anim.row, idle_anim.frame(), glm::vec2(-24.0f, 0.0f));
	//		sprites.draw(walk_anim.row, walk_anim.frame(), glm::vec2(24.0f, 0.0f), true); // flipped
	//	}

	{
		DrawLines lines(world_to_clip);

		AABB world_player_aabb = player.collider.getWorldSpaceAABB();
		glm::vec3 player_center = (world_player_aabb.min + world_player_aabb.max) * 0.5f;
		glm::vec3 player_extents = (world_player_aabb.max - world_player_aabb.min) * 0.5f;

		glm::mat4x3 player_draw_transform = glm::translate(glm::mat4(1.0f), player_center) * glm::scale(glm::mat4(1.0f), player_extents);

		lines.draw_box(player_draw_transform, glm::u8vec4(0, 255, 0, 255));

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
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDrawBuffer(GL_BACK);
	glViewport(0, 0, drawable_size.x, drawable_size.y);

	glDisable(GL_FRAMEBUFFER_SRGB);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glBlitFramebuffer(0, 0, PixelWidth, PixelHeight, 0, 0, drawable_size.x, drawable_size.y, GL_COLOR_BUFFER_BIT, GL_NEAREST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glEnable(GL_FRAMEBUFFER_SRGB);

	GL_ERRORS();
}

glm::vec3 PlayMode::get_leg_tip_position()
{
	// the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}

Scene::Transform *PlayMode::addLevelSprite(SpriteSheet const &sheet, glm::vec2 bl, bool is_fg, std::optional<AABB> obj_AABB)
{
	scene.transforms.emplace_back();
	Scene::Transform *sprite_transform = &scene.transforms.back();

	sprite_transform->position = glm::vec3(bl / PPU, 0.0f);

	level_sprites.push_back(StaticSprite{&sheet, sprite_transform, is_fg});

	if (obj_AABB.has_value())
	{
		Collider collider;
		collider.transform = sprite_transform;

		collider.local_aabb.min = obj_AABB->min / PPU;
		collider.local_aabb.max = obj_AABB->max / PPU;

		collider.layer = CollisionLayer::Level;
		collider.collision_mask = static_cast<std::uint32_t>(CollisionLayer::Player) | static_cast<std::uint32_t>(CollisionLayer::Enemy);

		colliders.push_back(collider);
	}

	return sprite_transform;
}