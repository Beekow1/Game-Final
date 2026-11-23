#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include "Player.hpp"
#include "Enemy.hpp"
#include "GameManager.hpp"
#include "Items.hpp"

#include "SpriteAnimator.hpp"
#include "SpriteSheet.hpp"

#include "ProcGen.hpp"
#include "RoomLayout.hpp"

#include "load_save_png.hpp"
#include "Light.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <optional>
#include <ranges>

struct PlayMode : Mode
{
	PlayMode(bool tutorial);
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void drawChunk(RoomLayout const &layout, glm::vec2 room_origin, int doors);

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, J;

	// level / respawn
	bool use_tutorial = false;
	glm::vec2 room_size = glm::vec2(0.0f); // camera clamp bounds
	glm::vec2 spawn_pos = glm::vec2(0.0f); // player center
	struct WorldEnemySpawn
	{
		EnemyType type;
		glm::vec2 position;
	};

	std::vector<WorldEnemySpawn> enemy_spawns;
	bool player_dead = false;

	// death zones
	std::vector<AABB> death_zones; 
	float void_y = -2.0f; 

	// item pop
	ItemId pickup_item = ItemId::MaxHP;
	bool pickup_was_random = false;
	float pickup_timer = 0.0f;
	static constexpr float PickupDuration = 1.2f;

	// shop structure
	struct ShopSlot
	{
		glm::vec2 pos; // world units, lowest point of item
		int price = 0;
		ItemId item = ItemId::MaxHP;
		bool sold = false;
	};
	std::vector<ShopSlot> shop_slots; 
	std::optional<AABB> shop_interior; // world units, camera lock
	glm::vec2 shopkeeper_pos = glm::vec2(0.0f);
	int hovered_slot = -1; 
	bool buy_pressed = false;
	float denied_timer = 0.0f; 
	float anim_time = 0.0f;

	bool was_in_shop = false;

	void buildShop(glm::vec2 origin_px);

	// exit door
	std::optional<AABB> exit_zone; // world units, empty = no exit 

	void respawn();

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Player player;
	ProcGen proc_gen;
	GenParams gen_params;
	float quota = 0.4f;

	float total_time = 0.0f;

	std::vector<std::unique_ptr<Enemy>> enemies;

	bool ghost_spawned = false;

	// Radioactive aura: stacks scale both radius and damage
	float rad_timer = 0.0f;
	static constexpr float RadBaseRadius = 4.0f;     // world units (tiles) with 1 stack
	static constexpr float RadRadiusPerStack = 2.0f; // added per extra stack
	static constexpr int RadDamagePerStack = 3;      // damage per second

	int radStacks() const { return GameManager::Get().itemCount(ItemId::Radioactive); }
	float radRadius() const { return RadBaseRadius + RadRadiusPerStack * float(std::max(0, radStacks() - 1)); }
	int radDamage() const { return RadDamagePerStack * radStacks(); }

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
	static constexpr int MAX_LIGHTS = 8;
	static constexpr int MAX_LIGHT_BLOCKERS = 128;
	static constexpr float PPU = 16.0f;

	SpriteAnimator player_anim;
	bool facing_left = false;

	std::vector<Collider> colliders;

	glm::vec2 input_dir = glm::vec2(0.0f);

	// car honk sound:
	std::shared_ptr<Sound::PlayingSample> dash_oneshot;
	std::shared_ptr<Sound::PlayingSample> jump_oneshot;
	std::shared_ptr<Sound::PlayingSample> attack_hit_oneshot;
	std::shared_ptr<Sound::PlayingSample> step_oneshot;

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

	std::vector<Light> room_lights;
	void addRoomLights(RoomLayout const &layout, glm::vec2 origin_px);

	GLuint fbo = 0;
	GLuint color_texture = 0;
	GLuint lighting_program = 0;
	GLuint lighting_vao = 0;
	
};
