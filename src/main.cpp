#include "game.hpp"
#include <iostream>
#include <ranges>
#include <random>

constexpr float W = 800, H = 600;

float clamp(float val, float mi, float ma) {
  if(val < mi) return mi;
  if(val > ma) return ma;
  return val;
}

struct LowResolutionPolicy : public Policy {
  const int pos_steps = 10;
  const int vel_steps = 6;
  const int num_of_actions = 8;
  std::vector<float> q_value;
  std::vector<float> _temp_buff;
  std::default_random_engine eng;

  const std::array<int, 6> dim_siz = {
    pos_steps, pos_steps, vel_steps, vel_steps, pos_steps, pos_steps
  };

  LowResolutionPolicy() : eng(43298742LL) {
    int siz = get_state_index({ int(1e9), int(1e9), int(1e9), int(1e9), int(1e9), int(1e9) }) + num_of_actions;
    _temp_buff.resize(siz, 0.0);
    q_value.resize(siz);
    for(float& val : q_value) {
      val = std::uniform_real_distribution(0.0, 1e-18)(eng);
    }
  }

  int get_state_index(GameState const& state) {
    std::array<int, 6> index;
    // State discretization. Using knowledge about W, H and maximum values of speed to make it meaningful
    index[0] = (int) round((clamp(state[0] / W, -0.5, 0.5) + 0.5) * (pos_steps - 1)); // px
    index[1] = (int) round((clamp(state[1] / H, -0.5, 0.5) + 0.5) * (pos_steps - 1)); // py
  
    index[2] = (int) round((clamp(state[2] / VX_LIM, -1, 1) + 1) / 2 * (vel_steps - 1)); // vel_x
    index[3] = (int) round((clamp(state[3] / JUMP_VEL, -3, 1) + 2) / 3 * (vel_steps - 1)); // vel_y
  
    index[4] = (int) round((clamp(state[4] / W, -0.5, 0.5) + 0.5) * (2 * pos_steps - 1)); // fx
    index[5] = (int) round((clamp(state[5] / H, -0.5, 0.5) + 0.5) * (2 * pos_steps - 1)); // fy
  
    int state_index = 0, mul = num_of_actions;
  
    for(int i = 0; i < 6; i++) {
      state_index += index[i] * mul;
      mul *= dim_siz[i];
    }

    return state_index;
  }

  GameAction game_action_from_mask(int mask) {
    GameAction res;
    std::get<0>(res) = !!(mask & (1));
    std::get<1>(res) = !!(mask & (2));
    std::get<2>(res) = !!(mask & (4));
    return res;
  }

  const double alpha = 0.05;
  const double gamma = 0.9;
  double eps = 0.9;

  int select_action(GameState state, PolicyMode _) {
    int state_index = get_state_index(state), best_act_mask = 0;
    if(std::uniform_real_distribution<float>(0, 1)(eng) > eps) {
      best_act_mask = std::uniform_int_distribution(0, int(1 << num_of_actions) - 1)(eng);
    }
    else {
      for(int i = 0; i < int(1 << num_of_actions); i++) {
        if(q_value[state_index + i] > q_value[state_index + best_act_mask]) {
          best_act_mask = i;
        }
      }
    }
    return best_act_mask;
  }

  //                    state, action_mask, reward
  std::vector<std::tuple<GameState, int, float>> episode;
  
  virtual GameAction operator()(GameState const& state, PolicyMode mode) override {
    int act_mask = select_action(state, mode);
    if(mode == PolicyMode::LEARNING_MODE) {
      episode.emplace_back(state, act_mask, 0);
    }
    if(int(episode.size()) > 100'000) {
      throw std::runtime_error("Too long episode!");
    }
    return game_action_from_mask(act_mask);
  }

  void learn() {
    float sum = 0.0;
    for(auto const& [ state, action, reward ] : episode | std::views::reverse)  {
      int q_value_ind = get_state_index(state) + action;
      sum = reward + gamma * sum;
      _temp_buff[q_value_ind] = sum;
    }
    for(auto const& [ state, action, reward ] : episode)  {
      int q_value_ind = get_state_index(state) + action;
      if(_temp_buff[q_value_ind] != 0.0) {
        q_value[q_value_ind] = (1 - alpha) * q_value[q_value_ind] + alpha * _temp_buff[q_value_ind];
        _temp_buff[q_value_ind] = 0.0;
      }
    }
    episode.clear();
  }

  void report() const {}

  virtual void get_feedback(float reward, bool episode_ended, PolicyMode mode) {
    std::get<2>(episode.back()) = reward;
    if(episode_ended) {
      if(mode == PolicyMode::LEARNING_MODE) {
        learn();
      }
      else {
        report();
      }
    }
  }

};

int main() {
  LowResolutionPolicy policy;

  {
    GameEnvironment train_env(W, H, 20);
    Game train_game(train_env);

    // 100k, 300k, 500k, ->1M<-, 2M, 10M
    const int NUM_EPISODES = 100'000;
    const double step = (1 - policy.eps) / NUM_EPISODES;
    for(int episode = 1; episode <= NUM_EPISODES; episode++) {
      std::cerr << "EPISODE: " << episode << '\n';
      train_env.simulated_run(policy, 10, PolicyMode::LEARNING_MODE);
      policy.eps += step;
    }
  }
  
  GameEnvironment test_env(W, H, 60);
  Game test_game(test_env);
  test_env.run(&policy);

  return 0;
}