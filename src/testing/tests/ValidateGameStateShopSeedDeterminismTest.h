#pragma once

#include "../../components/is_dish.h"
#include "../../components/is_shop_item.h"
#include "../../dish_types.h"
#include "../../game_state_manager.h"
#include "../../query.h"
#include "../../seeded_rng.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <vector>

namespace ValidateGameStateShopSeedDeterminismTestHelpers {

static std::vector<DishType> read_shop_types() {
  std::vector<DishType> types;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    if (!entity.cleanup) {
      types.push_back(entity.get<IsDish>().type);
    }
  }
  return types;
}

} // namespace ValidateGameStateShopSeedDeterminismTestHelpers

TEST(validate_game_state_shop_seed_determinism) {
  using namespace ValidateGameStateShopSeedDeterminismTestHelpers;
  app.launch_game();
  app.navigate_to_shop();
  app.wait_for_frames(20);

  app.once([&] {
    std::vector<DishType> initial_shop_items = read_shop_types();
    app.expect_not_empty(initial_shop_items, "initial shop items should exist");
    app.set_test_int("initial_count", static_cast<int>(initial_shop_items.size()));
    for (size_t i = 0; i < initial_shop_items.size(); ++i) {
      app.set_test_int("initial_type_" + std::to_string(i),
                       static_cast<int>(initial_shop_items[i]));
    }
    const uint64_t seed = app.read_shop_seed();
    app.set_test_int("seed_lo", static_cast<int>(seed & 0xFFFFFFFFu));
    app.set_test_int("seed_hi", static_cast<int>(seed >> 32));
  });

  app.trigger_game_state_save();
  app.wait_for_frames(5);
  app.expect_true(app.save_file_exists(), "save file should exist");

  app.once([&] {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsShopItem>()
             .whereHasComponent<IsDish>()
             .gen()) {
      entity.cleanup = true;
    }
  });
  app.wait_for_frames(5);

  app.once([&] {
    app.expect_empty(read_shop_types(), "shop should be empty after clearing");
  });

  app.trigger_game_state_load();
  app.wait_for_frames(10);

  const uint64_t restored_seed = app.read_shop_seed();
  const uint64_t initial_seed =
      (static_cast<uint64_t>(
           static_cast<uint32_t>(app.get_test_int("seed_hi").value()))
       << 32) |
      static_cast<uint32_t>(app.get_test_int("seed_lo").value());
  app.expect_eq(restored_seed, initial_seed, "shop seed should be restored");

  app.wait_for_frames(20);

  std::vector<DishType> restored_shop_items = read_shop_types();
  const int initial_count = app.get_test_int("initial_count").value();
  if (!restored_shop_items.empty()) {
    app.expect_count_eq(static_cast<int>(restored_shop_items.size()),
                        initial_count,
                        "restored shop item count should match initial");
    int matching_items = 0;
    for (int i = 0;
         i < std::min(static_cast<int>(restored_shop_items.size()), initial_count);
         ++i) {
      if (static_cast<int>(restored_shop_items[i]) ==
          app.get_test_int("initial_type_" + std::to_string(i)).value()) {
        matching_items++;
      }
    }
    app.expect_count_gt(matching_items, 0,
                        "some shop items should match due to seed determinism");
  }
}
