#pragma once

#include "rl.h"
#include <afterhours/src/singleton.h>
#include <cstdint>
#include <random>

/**
 * SeededRng - Deterministic random number generator singleton
 *
 * Provides a deterministic RNG that can be seeded for reproducible results.
 * Used for shop generation, battle simulation, and other game systems that
 * require deterministic randomness.
 */
SINGLETON_FWD(SeededRng)
struct SeededRng {
  SINGLETON(SeededRng)

  std::mt19937_64 gen;
  uint64_t seed = 0;

  SeededRng() : gen(0) {}

  /**
   * Seed the RNG with a specific value
   */
  void set_seed(uint64_t new_seed) {
    seed = new_seed;
    gen.seed(seed);
  }

  /**
   * Generate a new seed using random_device and set it
   * Use this for non-deterministic initialization (e.g., shop on game start)
   */
  void randomize_seed() {
    std::random_device rd;
    seed = rd();
    gen.seed(seed);
  }

  /**
   * Generate a random index in [0, size)
   * Useful for selecting random items from containers
   */
  template <typename T> size_t gen_index(T size) {
    if (size == 0)
      return 0;
    std::uniform_int_distribution<size_t> dis(0, size - 1);
    return dis(gen);
  }

  /**
   * Generate a random integer in [min, max] (inclusive)
   */
  template <typename T> T gen_in_range(T min, T max) {
    std::uniform_int_distribution<T> dis(min, max);
    return dis(gen);
  }

  /**
   * Generate a random float in [min, max)
   */
  float gen_float(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
  }

  /**
   * Generate a random integer from [0, max) (exclusive of max)
   * Legacy compatibility with rand() % max pattern
   */
  template <typename T> T gen_mod(T max) {
    if (max <= 0)
      return 0;
    std::uniform_int_distribution<T> dis(0, max - 1);
    return dis(gen);
  }

  /**
   * Generate a random vec2 within a rectangle
   */
  vec2 vec_rand_in_box(const Rectangle &rect) {
    return vec2{
        rect.x + gen_float(0.0f, rect.width),
        rect.y + gen_float(0.0f, rect.height),
    };
  }

  /**
   * Generate a truly random number using random_device (non-seeded)
   * Use this when you need actual randomness, not deterministic seeded
   * randomness
   */
  static uint64_t get_actually_random_number_random_seed() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    return dis(gen);
  }
};
