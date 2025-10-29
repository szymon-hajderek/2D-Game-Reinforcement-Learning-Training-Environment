#pragma once
#include "env.hpp"

// -----------------------------------------------------------------------------
// Constants - radiuses of player and of the fruit */
  const float PR = 40.f;
  const float FR =  5.f;
// -----------------------------------------------------------------------------
// --- Initialization of the game ----------------------------------------------
// -----------------------------------------------------------------------------
void Game::init() {
  fx  = random_in_range(-env.width / 4 + FR,  env.width / 4 - FR);
  fy  = random_in_range(-env.height / 4 + FR, env.height / 4 - FR);
  // px = random_in_range(-300.f, 300.f);
  // py = random_in_range(-200.f, 200.f);
}

// --- Drawing the game (happens every frame) ----------------------------------
void Game::render() {
    sf::CircleShape fruit(FR);
    fruit.setFillColor(sf::Color::Green);
    fruit.setPosition({fx + env.width / 2 - FR, env.height / 2 - fy - FR});
    env.get_win().draw(fruit);

    sf::CircleShape player(PR);
    player.setFillColor(sf::Color::White);
    player.setPosition({px + env.width / 2 - PR, env.height / 2 - py - PR});
    env.get_win().draw(player);
}

// --- Game constants ----------------------------------------------------------
  constexpr static float G        = 2000.f;
  constexpr static float JUMP_VEL =  500.f;
  constexpr static float X_ACC    = 1000.f;
  constexpr static float VX_LIM   = 1000.f;
    
// --- Game mechanics (called every frame) ------------------------------------
// returns `GameEvents` object, which is then used to calculate rewards.
// updates the game state, knowing that dt seconds elapsed, since last frame.
// ------- What you may want to use: ------------------------------------------
//   - env.get_player_action() <-> `GameAction` object.
//     Contains actions currently performed by the player or by the agent.
//   - env.width, env.height <-> width and height of the screen.
//     The point (0, 0) is the top left corner.
//   - env.get_win() <-> the window to draw on.
// ----------------------------------------------------------------------------
GameEvents Game::update(float dt) {
  auto [ up, left, right ] = env.get_player_action();

  // Checking the input and acting accordingly
  if(up) pvy = JUMP_VEL; else pvy -= G * dt;
  if(left)  pvx -= X_ACC * dt;
  if(right) pvx += X_ACC * dt;

  // Capping the velocity at an maximum value
  if(std::abs(pvx) > VX_LIM) pvx = std::copysign(VX_LIM, pvx);

  // Once the velocity changes direction, it should be set to zero (no flickering almost in-place)
  if(last_vx != 0 && std::copysign(1.f, pvx) != std::copysign(1.f, last_vx))
    pvx = 0;

  // "Friction"-like behaviour
  if(!(left || right) && pvx != 0)
    pvx -= std::copysign(X_ACC * dt, pvx);

  // Left / right boundaries of the box
  if(px >  env.width/2 - PR) { px =  env.width/2 - PR; pvx *= -0.8f; }
  if(px < -env.width/2 + PR) { px = -env.width/2 + PR; pvx *= -0.8f; }

  // Moving accroding to the velocity
  py += pvy * dt;
  px += pvx * dt;
  last_vx = pvx;

  // Top / bottom boundaries of the box
  // if boundary is touched, then player's death should be reported by setting: `ev.died = 1`
  GameEvents ev;
  if(py < -env.height/2 + PR || py > env.height/2 - PR) {
    pvx = pvy = px = py = 0;
    ev.died = 1;
  }

  // Checking for collision with the fruit (circle vs circle collision)
  // if the fruit is touched, then the fruit collection should be reported by setting: `ev.fruit_collected = 1`
  const float dx = px - fx, dy = py - fy;
  if(dx * dx + dy * dy <= (PR + FR) * (PR + FR)) {
    // New fruit position is randomly chosen 
    fx = random_in_range(-env.width /4 + FR,  env.width /4 - FR);
    fy = random_in_range(-env.height/4 + FR, env.height/4 - FR);
    ev.fruit_collected = 1;
  }

  return ev;
}

// --- Reward constants --------------------------------------------------------
constexpr int FRUIT_COLLECTED_REWARD = 2000;
constexpr int DIED_REWARD = -5000;

// --- Reward defintiion -------------------------------------------------------
float GameEnvironment::calc_rewards(GameEvents const& ev) {
  return ev.died * DIED_REWARD + ev.fruit_collected * FRUIT_COLLECTED_REWARD;
}

// --- HUD update --------------------------------------------------------------
void GameEnvironment::update_hud() {
  info_string = "wynik: " + std::to_string(deaths * DIED_REWARD + score * FRUIT_COLLECTED_REWARD);
}