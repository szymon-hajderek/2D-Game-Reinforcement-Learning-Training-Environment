#include "game.hpp"
#include <iostream>
#include <ranges>
#include <random>

constexpr float W = 800, H = 600;


struct DumbPolicy : public Policy {
  std::default_random_engine eng;

  DumbPolicy() : eng(43298742LL) {}
  
  virtual GameAction operator()(GameState const& state, PolicyMode mode) override {
    return GameAction {
      std::uniform_int_distribution<int>(0, 10)(eng) == 3,
      std::uniform_int_distribution<int>(0, 1)(eng),
      std::uniform_int_distribution<int>(0, 1)(eng)
    };
  }

  virtual void get_feedback(float reward, bool episode_ended, PolicyMode mode) override {
    // I am a super cool Policy. I don't need feedback.
  }

};

int main() {
  int runs = 300;
  int secs = 30;
  int fps = 60;
  double avg = 0.0;
  DumbPolicy* policy = new DumbPolicy();
  for(int i = 1; i <= runs; i++) {
    GameEnvironment test_env(W, H, 60);
    Game test_game(test_env);
    test_env.simulated_run(*policy, secs, PolicyMode::INFERENCE_MODE);
    avg += test_env.get_score();
  }
  avg /= runs;
  std::cerr << "Average score: " << avg << '\n';
}
