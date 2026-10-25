#include "PlayMode.hpp"
#include "SpriteSheet.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "DrawSprites.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("hexapod.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("hexapod.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > dusty_floor_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("dusty-floor.opus"));
});


Load< Sound::Sample > honk_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("honk.wav"));
});

Load< SpriteSheet > test_character_sheet(LoadTagDefault, []() -> SpriteSheet const * {
	return new SpriteSheet(data_path("sprites/test-character.png"), SpriteSheet::Info{
		.cell_size = glm::uvec2(16, 24), 
		.rows = {
				{"idle", 2, 4.0f, true},
				{"walk", 4, 10.0f, true},
		},
	});
});


PlayMode::PlayMode() : scene(*hexapod_scene), 
	idle_anim(*test_character_sheet, "idle"),
	walk_anim(*test_character_sheet, "walk") {

	colliders.reserve(10);

	scene.transforms.emplace_back();
	Scene::Transform *player_transform = &scene.transforms.back();

	player_transform->position = glm::vec3(0.0f, 0.0f, 0.0f);
	player.transform = player_transform;
	player.collider.transform = player_transform;
	
	player.collider.local_aabb.min = glm::vec3(-0.5f, -0.5f, -0.5f);
  player.collider.local_aabb.max = glm::vec3(0.5f, 0.5f, 0.5f);

	colliders.emplace_back(player.collider);

	floor.local_aabb.min = glm::vec3(-10.0f, -3.0f, 0.0f);
  floor.local_aabb.max = glm::vec3(10.0f, 3.0f, 0.0f);

	scene.transforms.emplace_back();
	floor_transform = &scene.transforms.back();

	floor_transform->position = glm::vec3(0.0f, -6.0f, 0.0f);
	floor.transform = floor_transform;

	colliders.emplace_back(floor);

	//get pointers to leg for convenience:
	for (auto &transform : scene.transforms) {
		if (transform.name == "Hip.FL") hip = &transform;
		else if (transform.name == "UpperLeg.FL") upper_leg = &transform;
		else if (transform.name == "LowerLeg.FL") lower_leg = &transform;
	}
	if (hip == nullptr) throw std::runtime_error("Hip not found.");
	if (upper_leg == nullptr) throw std::runtime_error("Upper leg not found.");
	if (lower_leg == nullptr) throw std::runtime_error("Lower leg not found.");

	hip_base_rotation = hip->rotation;
	upper_leg_base_rotation = upper_leg->rotation;
	lower_leg_base_rotation = lower_leg->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	//start music loop playing:
	// (note: position will be over-ridden in update())
	leg_tip_loop = Sound::loop_3D(*dusty_floor_sample, 1.0f, get_leg_tip_position(), 10.0f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			input_dir += glm::vec2(-1.0f, 0.0f);
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			input_dir += glm::vec2(1.0f, 0.0f);
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			input_dir += glm::vec2(0.0f, 1.0f);
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			input_dir += glm::vec2(0.0f, -1.0f);
			return true;
		} else if (evt.key.key == SDLK_J) {
			J.downs += 1;
			J.pressed = true;
			player.dash(input_dir);
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			player.jump_not_wasted = true;
			player.jump_buffer_timer = player.jump_buffer_duration;
			//if (honk_oneshot) honk_oneshot->stop();
			//honk_oneshot = Sound::play_3D(*honk_sample, 0.3f, glm::vec3(4.6f, -7.8f, 6.9f)); //hardcoded position of front of car, from blender
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			input_dir -= glm::vec2(-1.0f, 0.0f);
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			input_dir -= glm::vec2(1.0f, 0.0f);
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			input_dir -= glm::vec2(0.0f, 1.0f);
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			input_dir -= glm::vec2(0.0f, -1.0f);
			return true;
		} else if (evt.key.key == SDLK_J) {
			J.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_SPACE) {
			player.jump_not_wasted = false;
			player.attemptJumpCut();
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			//SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		return true;
	}

	return false;
}

void PlayMode::update(float elapsed) {

	//slowly rotates through [0,1):
	wobble += elapsed / 10.0f;
	wobble -= std::floor(wobble); 

	player.jump_buffer_timer -= elapsed;

	if (!player.is_grounded) { player.coyote_timer -= elapsed; }

	//move sound to follow leg tip position:
	leg_tip_loop->set_position(get_leg_tip_position(), 1.0f / 60.0f);

	{ //update listener to camera position:
		glm::mat4x3 frame = camera->transform->make_parent_from_local();
		glm::vec3 frame_right = frame[0];
		glm::vec3 frame_at = frame[3];
		Sound::listener.set_position_right(frame_at, frame_right, 1.0f / 60.0f);
	}

	player.move(input_dir, elapsed);

	player.updatePlayer(elapsed);

	runCollision(player, colliders);

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	idle_anim.update(elapsed);
	walk_anim.update(elapsed);
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	// 640 x 360 game window
	const glm::vec2 GameSize(640.0f, 360.0f);
	float scale = std::max(1.0f, std::round(drawable_size.y / 360.0f));
	glm::vec2 view = glm::vec2(drawable_size) / scale; //visible area

	glViewport(0, 0, drawable_size.x, drawable_size.y);
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//game area background:
	glm::vec2 lo = glm::round(view_center) - glm::floor(view / 2.0f);
	glm::mat4 world_to_clip = glm::ortho(lo.x, lo.x + view.x, lo.y, lo.y + view.y);

	{ //sprites
		DrawSprites sprites(*test_character_sheet, world_to_clip);
		sprites.draw(idle_anim.row, idle_anim.frame(), glm::vec2(-24.0f, 0.0f));
		sprites.draw(walk_anim.row, walk_anim.frame(), glm::vec2( 24.0f, 0.0f), true); //flipped
	}

	{
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		float zoom = 10.0f;

		glm::mat4 world_to_clip = glm::ortho(-aspect * zoom, aspect * zoom, -zoom, zoom, 0.0f, 1.0f);

		DrawLines lines(world_to_clip);

		AABB world_floor_aabb = floor.getWorldSpaceAABB();
        glm::vec3 floor_center = (world_floor_aabb.min + world_floor_aabb.max) * 0.5f;
        glm::vec3 floor_extents = (world_floor_aabb.max - world_floor_aabb.min) * 0.5f;
        glm::mat4x3 floor_draw_transform = glm::translate(glm::mat4(1.0f), floor_center) 
                                         * glm::scale(glm::mat4(1.0f), floor_extents);
        lines.draw_box(floor_draw_transform, glm::u8vec4(255, 0, 0, 255));


		AABB world_player_aabb = player.collider.getWorldSpaceAABB();
        glm::vec3 player_center = (world_player_aabb.min + world_player_aabb.max) * 0.5f;
        glm::vec3 player_extents = (world_player_aabb.max - world_player_aabb.min) * 0.5f;

		glm::mat4x3 player_draw_transform = glm::translate(glm::mat4(1.0f), player_center) 
                                          * glm::scale(glm::mat4(1.0f), player_extents);
                                          
        lines.draw_box(player_draw_transform, glm::u8vec4(0, 255, 0, 255));
	}
	glViewport(0, 0, drawable_size.x, drawable_size.y);

	GL_ERRORS();
}


glm::vec3 PlayMode::get_leg_tip_position() {
	//the vertex position here was read from the model in blender:
	return lower_leg->make_world_from_local() * glm::vec4(-1.26137f, -11.861f, 0.0f, 1.0f);
}
