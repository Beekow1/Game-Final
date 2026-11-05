#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include "Player.hpp"
#include "Enemy.hpp"

#include "SpriteAnimator.hpp"
#include "SpriteSheet.hpp"

#include "ProcGen.hpp"
#include "RoomTemplate.hpp"

#include "load_save_png.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <optional>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void drawChunk(RoomTemplate template_room, glm::vec2 room_origin);

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, J;

	// level / respawn
	bool use_tutorial = true;
	glm::vec2 room_size = glm::vec2(0.0f); // camera clamp bounds
	glm::vec2 spawn_pos = glm::vec2(0.0f); // player center
	std::vector<glm::vec2> dummy_spawns;   // enemy centers
	bool player_dead = false;
	float kill_y = -2.0f; // units below the room's bottom edge

	void respawn();

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Player player;
	ProcGen proc_gen;
	GenParams gen_params;

	std::vector<std::unique_ptr<Enemy>> enemies;

	struct StaticSprite
	{
		SpriteSheet const *sheet = nullptr;
		Scene::Transform *transform = nullptr;
		bool is_fg = false;
	};

	std::vector<StaticSprite> level_sprites;

	Scene::Transform *addLevelSprite(SpriteSheet const &sheet, glm::vec2 bl, bool is_fg = false, std::vector<AABB> const &obj_AABBs = {});

	// game render settings
	static constexpr int PixelWidth = 480;
	static constexpr int PixelHeight = 270;
	static constexpr float PPU = 16.0f;

	SpriteAnimator player_anim;
	bool facing_left = false;

	std::vector<Collider> colliders;

	glm::vec2 input_dir = glm::vec2(0.0f);

	// car honk sound:
	std::shared_ptr<Sound::PlayingSample> honk_oneshot;

	// camera:
	Scene::Camera *camera = nullptr;
	glm::vec2 cam_pos;
	glm::vec2 target_cam_pos;
	float look_ahead_dir = 1.0f;
	float look_ahead = 1.2f;
	float cam_smoothing = 0.08f;
	float dead_zone_x = 0.5f;
	float dead_zone_y = 3.0f;
	float pull_strength = 4.0f;

	GLuint fbo = 0;
	GLuint color_texture = 0;
};
