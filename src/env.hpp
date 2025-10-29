#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <cmath>
#include <optional>
#include <tuple>
#include <random>
#include <string>

// --- Forward declaration -----------------------------------------------------
struct Game;

// --- Useful aliasses ---------------------------------------------------------
using GameState = std::array<float, 6>;
using GameAction = std::tuple<bool, bool, bool>;

// --- GameEvents --------------------------------------------------------------
struct GameEvents {
  int fruit_collected{0};
  int died{0};
};

// --- Mode --------------------------------------------------------------------
enum struct PolicyMode { INFERENCE_MODE, LEARNING_MODE, };

// --- Policy interface --------------------------------------------------------
struct Policy {
  virtual GameAction operator()(GameState const&, PolicyMode) = 0;
  virtual void get_feedback(float, bool, PolicyMode) = 0;
  virtual ~Policy()=default;
};

// --- Helpers -----------------------------------------------------------------
float random_in_range(float a,float b) {
  static std::mt19937 rng{std::random_device{}()};
  return std::uniform_real_distribution<float>(a, b)(rng);
}

// --- GameEnvironment ---------------------------------------------------------
// - Renders any useful information (Text-based debug info)
// - Ensures proper Policy / Game interactions
// - Defines and calculates Rewards

class GameEnvironment {
  GameAction player_action;
  sf::RenderWindow win;
  sf::Clock clk;
  Game* game{nullptr};
  std::string info_string;
  unsigned FPS;

  int deaths{0}, score{0};
  float dt{0.f};

 public:
  float width, height;

  GameAction const& get_player_action() {
    return player_action;
  }

  sf::RenderWindow& get_win() {
    return win;
  }

  int get_score() {
    return score;
  }

  GameEnvironment(float w, float h, unsigned _FPS = 20) {
    win = sf::RenderWindow(sf::VideoMode({static_cast<unsigned>(w),static_cast<unsigned>(h)}), "oOoOo", sf::State::Windowed);
    width = w; height = h, FPS = _FPS;
    win.setFramerateLimit(FPS);
  }

  void register_game(Game* g) { game = g; }
  float calc_rewards(const GameEvents& ev);
  void update_hud();
  void resolve_actions(Policy* policy, PolicyMode mode);
  void run(Policy* policy = nullptr, PolicyMode mode = PolicyMode::INFERENCE_MODE, bool should_render=true);
  float simulated_run(Policy& policy, int seconds=10,  PolicyMode mode = PolicyMode::LEARNING_MODE);
};

// --- Game --------------------------------------------------------------------
// - Provides game mechanics
// - Provides 'events' used by GameEnvironment to give rewards
// - Renders in-game objects in the window: `env.get_win()`

struct Game {
  GameEnvironment& env;
  float last_vx{0};

  /* state known by the policy */
  float px{0}, py{0}, pvx{0}, pvy{0}, fx{0}, fy{0};

  GameState get_state() const;

  void init();

  Game(GameEnvironment& e) : env{e} {
    env.register_game(this);
    init();
  }

  void render();
  GameEvents update(float dt);

};

// --- GameEnvironment (definitions) ------------------------------------------
void GameEnvironment::run(Policy* policy, PolicyMode mode, bool should_render) {
  sf::Font font; font.openFromFile("lato.ttf");
  sf::Text hud(font, "", 20); hud.setPosition({16.f, 16.f}); hud.setFillColor(sf::Color::White);

  while(win.isOpen()) {
    while(const std::optional<sf::Event> e = win.pollEvent())
      if(e->is<sf::Event::Closed>()) win.close();

    resolve_actions(policy, mode);

    if(should_render) win.clear({ 122, 122, 122 });

    GameEvents ge = game->update(dt);
    deaths += ge.died;
    score  += ge.fruit_collected;

    if(policy) policy->get_feedback(calc_rewards(ge), !win.isOpen(), mode);

    if(should_render) {
      update_hud();
      hud.setString(info_string);
      game->render();
      win.draw(hud);
      win.display();
    }
    dt = clk.restart().asSeconds();
  }
}

float GameEnvironment::simulated_run(Policy& policy,int seconds, PolicyMode mode) {
  const int updates = seconds * FPS;
  const float fixed_dt = 1.f / FPS;
  float total = 0.f;

  for(int i=1;i<=updates;++i) {
    resolve_actions(&policy, mode);
    GameEvents ge = game->update(fixed_dt);
    float r = calc_rewards(ge);
    total += r;
    policy.get_feedback(r, i == updates, mode);
  }
  return total;
}
// --- (Policy -> action) mapping ----------------------------------------------
void GameEnvironment::resolve_actions(Policy* policy, PolicyMode mode) {
  if(!policy) {
    player_action = {
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up),
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left),
      sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)
    };
  } else {
    player_action = (*policy)(game->get_state(), mode);
  }
}

// --- Defining what policy gets as its input ----------------------------------
GameState Game::get_state() const {
  return { px, py, pvx, pvy, fx, fy };
}