#pragma once

#include "../seeded_rng.h"
#include "../server/file_storage.h"
#include <afterhours/ah.h>
#include <afterhours/src/singleton.h>
#include <chrono>
#include <cstdint>
#include <fmt/format.h>
#include <string>

SINGLETON_FWD(UserId)
struct UserId : afterhours::BaseComponent {
  SINGLETON(UserId)

  std::string userId;

  UserId() { load_or_generate(); }

private:
  void load_or_generate() {
    userId =
        server::FileStorage::load_string_from_file("output/saves/user_id.txt");
    if (userId.empty()) {
      generate_and_save();
    }
    log_info("Logged in as {}", userId);
  }

  void generate_and_save() {
    auto timestamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    uint64_t random = SeededRng::get_actually_random_number_random_seed();
    userId = fmt::format("user_{}_{:x}", timestamp, random);
    server::FileStorage::save_string_to_file("output/saves/user_id.txt",
                                             userId);
  }
};
