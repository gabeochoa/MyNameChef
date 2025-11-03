#pragma once

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
};
