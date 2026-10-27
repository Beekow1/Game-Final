#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include "Player.hpp"
#include "Enemy.hpp"

#include "SpriteAnimator.hpp"
#include "SpriteSheet.hpp"

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

	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, J;

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	Player player;

	std::vector<std::unique_ptr<Enemy>> enemies;

	struct StaticSprite
	{
		SpriteSheet const *sheet = nullptr;
		Scene::Transform *transform = nullptr;
		bool is_fg = false;
	};

	std::vector<StaticSprite> level_sprites;

	Scene::Transform *addLevelSprite(SpriteSheet const &sheet, glm::vec2 bl, bool is_fg = false, std::optional<AABB> obj_AABB = std::nullopt);

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

	GLuint fbo = 0;
	GLuint color_texture = 0;
};
