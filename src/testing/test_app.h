#pragma once

#include "../components/battle_team_tags.h"
#include "../components/combat_stats.h"
#include "../components/cuisine_tag.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/pending_combat_mods.h"
#include "../components/persistent_combat_modifiers.h"
#include "../components/trigger_event.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <chrono>
#include <functional>
#include <optional>
#include <set>
#include <source_location>
#include <sstream>
#include <string>
#include <vector>

struct TestDishInfo {
  afterhours::EntityID id;
  DishType type;
  std::string name;
  int slot;
  int level;
  bool operator==(const TestDishInfo &other) const { return id == other.id; }
};

struct TestShopItemInfo {
  afterhours::EntityID id;
  DishType type;
  std::string name;
  int slot;
  int price;
  bool operator==(const TestShopItemInfo &other) const {
    return id == other.id;
  }
};

class TestDishBuilder {
  DishType type;
  DishBattleState::TeamSide team_side = DishBattleState::TeamSide::Player;
  int slot = 0;
  DishBattleState::Phase phase = DishBattleState::Phase::InQueue;
  bool has_combat_stats = false;
  std::optional<int> level;
  std::optional<CuisineTagType> cuisine_tag;
  std::optional<std::pair<int, int>> persistent_modifier;

public:
  explicit TestDishBuilder(DishType type) : type(type) {}

  TestDishBuilder &on_team(DishBattleState::TeamSide side) {
    team_side = side;
    return *this;
  }

  TestDishBuilder &at_slot(int s) {
    slot = s;
    return *this;
  }

  TestDishBuilder &in_phase(DishBattleState::Phase p) {
    phase = p;
    return *this;
  }

  TestDishBuilder &with_combat_stats() {
    has_combat_stats = true;
    return *this;
  }

  TestDishBuilder &at_level(int lvl) {
    level = lvl;
    return *this;
  }

  TestDishBuilder &with_cuisine_tag(CuisineTagType cuisine) {
    cuisine_tag = cuisine;
    return *this;
  }

  TestDishBuilder &with_persistent_modifier(int zing, int body) {
    persistent_modifier = std::make_pair(zing, body);
    return *this;
  }

  afterhours::EntityID commit();
};

using TestOperationID = size_t;

struct TestApp {
  std::string current_test_name;
  std::string failure_message;
  std::string failure_location;
  bool game_launched = false;
  bool test_in_progress = false;
  bool test_resuming = false;
  bool test_executing = false; // Guard against recursive run_test calls
  std::set<TestOperationID> completed_operations;

  static TestOperationID
  generate_operation_id(const std::source_location &loc,
                        const std::string &context = "") {
    std::stringstream pre_hash;
    pre_hash << "file: " << loc.file_name() << '(' << loc.line() << ':'
             << loc.column() << ") `" << loc.function_name() << "`: " << context
             << '\n';
    return std::hash<std::string>{}(pre_hash.str());
  }

  // State for non-blocking waits in main game loop
  struct WaitState {
    enum Type { None, Screen, UI, FrameDelay };
    Type type = None;
    GameStateManager::Screen target_screen;
    std::string target_ui_label;
    float timeout_sec = 5.0f;
    std::chrono::steady_clock::time_point start_time;
    std::string location;
    int frame_delay_count = 0;
    TestOperationID operation_id = 0;
  } wait_state;

  // Continuation function for yield/resume pattern
  std::function<void()> yield_continuation;

  TestApp() = default;

  void set_test_name(const std::string &name) {
    // Only clear completed operations if test name actually changed
    // This prevents clearing completed waits when re-running the same test
    if (current_test_name != name) {
      current_test_name = name;
      completed_operations.clear();
    }
  }

  void fail(const std::string &message, const std::string &location = "");

  // Yield/resume support for one-time test execution
  void yield(std::function<void()> continuation);

  // Check wait conditions (call this from TestSystem each frame)
  bool check_wait_conditions();

  TestApp &launch_game(
      const std::source_location &loc = std::source_location::current());
  TestApp &
  click(const std::string &button_label,
        const std::source_location &loc = std::source_location::current());
  TestApp &navigate_to_shop(
      const std::source_location &loc = std::source_location::current());
  TestApp &navigate_to_battle(
      const std::source_location &loc = std::source_location::current());

  std::vector<TestDishInfo> read_player_inventory();
  std::vector<TestShopItemInfo> read_store_options();
  int read_wallet_gold();
  int read_player_health();
  int read_round();
  int read_shop_tier();
  struct RerollCostInfo {
    int base;
    int increment;
    int current;
  };
  RerollCostInfo read_reroll_cost();
  uint64_t read_shop_seed();
  int count_active_player_dishes();
  int count_active_opponent_dishes();
  bool read_replay_paused();

  // Functions to manually manipulate game state for testing
  // These bypass normal game logic to set up specific test scenarios
  TestApp &set_battle_speed(float speed);
  TestApp &set_wallet_gold(int gold, const std::string &location = "");
  TestApp &create_inventory_item(DishType type, int slot);
  TestApp &trigger_game_state_save();
  TestApp &trigger_game_state_load();
  bool save_file_exists();
  GameStateManager::Screen read_current_screen();

  TestApp &wait_for_ui_exists(
      const std::string &label, float timeout_sec = 5.0f,
      const std::string &location = "",
      const std::source_location &loc = std::source_location::current());
  TestApp &expect_screen_is(GameStateManager::Screen screen,
                            const std::string &location = "");
  TestApp &expect_inventory_contains(DishType type,
                                     const std::string &location = "");
  TestApp &expect_wallet_has(int gold, const std::string &location = "");
  bool can_afford_purchase(DishType type);
  bool try_purchase_item(DishType type, int inventory_slot = -1,
                         const std::string &location = "");
  TestApp &purchase_item(DishType type, int inventory_slot = -1,
                         const std::string &location = "");

  TestApp &wait_for_screen(
      GameStateManager::Screen screen, float timeout_sec = 5.0f,
      const std::source_location &loc = std::source_location::current());
  TestApp &wait_for_frames(int frames, const std::source_location &loc =
                                           std::source_location::current());
  TestApp &pump_frame();

  TestApp &setup_battle();
  TestDishBuilder create_dish(DishType type);
  TestApp &advance_battle_until_onserve_complete(float timeout_sec = 5.0f);
  template <typename T>
  TestApp &expect_dish_has_component(afterhours::EntityID dish_id,
                                     const std::string &location = "");
  TestApp &expect_flavor_mods(afterhours::EntityID dish_id,
                              const DeferredFlavorMods &expected,
                              const std::string &location = "");
  TestApp &expect_combat_mods(afterhours::EntityID dish_id,
                              const PendingCombatMods &expected,
                              const std::string &location = "");
  TestApp &expect_trigger_fired(TriggerHook hook, afterhours::EntityID dish_id,
                                const std::string &location = "");

  TestApp &expect_count_eq(int actual, int expected,
                           const std::string &description,
                           const std::string &location = "");
  TestApp &expect_count_gt(int actual, int min, const std::string &description,
                           const std::string &location = "");
  TestApp &expect_count_lt(int actual, int max, const std::string &description,
                           const std::string &location = "");
  TestApp &expect_count_gte(int actual, int min, const std::string &description,
                            const std::string &location = "");
  TestApp &expect_count_lte(int actual, int max, const std::string &description,
                            const std::string &location = "");

  template <typename T>
  TestApp &expect_eq(const T &actual, const T &expected,
                     const std::string &description,
                     const std::string &location = "");

  template <typename Container>
  TestApp &expect_not_empty(const Container &collection,
                            const std::string &description,
                            const std::string &location = "");
  template <typename Container>
  TestApp &expect_empty(const Container &collection,
                        const std::string &description,
                        const std::string &location = "");

  template <typename T>
  TestApp &expect_entity_has_component(afterhours::EntityID entity_id,
                                       const std::string &location = "");
  template <typename T>
  TestApp &expect_singleton_has_component(afterhours::RefEntity &singleton_opt,
                                          const std::string &component_name,
                                          const std::string &location = "");

  TestApp &expect_dish_phase(afterhours::EntityID dish_id,
                             DishBattleState::Phase expected_phase,
                             const std::string &location = "");
  TestApp &expect_dish_count(int expected_player, int expected_opponent,
                             const std::string &location = "");
  TestApp &expect_player_dish_count(int expected,
                                    const std::string &location = "");
  TestApp &expect_opponent_dish_count(int expected,
                                      const std::string &location = "");
  TestApp &expect_dish_count_at_least(int min_player, int min_opponent,
                                      const std::string &location = "");

  TestApp &expect_wallet_at_least(int min_gold,
                                  const std::string &location = "");
  TestApp &expect_wallet_between(int min_gold, int max_gold,
                                 const std::string &location = "");

  afterhours::OptEntity find_inventory_item_by_slot(int slot_index);
  afterhours::OptEntity find_drop_slot(int slot_id);
  int find_free_shop_slot();
  int find_free_inventory_slot();
  afterhours::OptEntity find_shop_item(afterhours::EntityID id, int slot);
  bool simulate_sell(afterhours::Entity &inventory_item);

  // Helper to find entity by ID (useful for tests that need entity references)
  afterhours::Entity *find_entity_by_id(afterhours::EntityID id);

  TestApp &wait_for_battle_initialized(float timeout_sec = 10.0f,
                                       const std::string &location = "");
  TestApp &wait_for_dishes_in_combat(int min_count = 1,
                                     float timeout_sec = 10.0f,
                                     const std::string &location = "");
  TestApp &wait_for_animations_complete(float timeout_sec = 5.0f,
                                        const std::string &location = "");
  TestApp &expect_combat_ticks_occurred(int min_ticks = 1,
                                        const std::string &location = "");
  TestApp &wait_for_battle_complete(float timeout_sec = 60.0f,
                                    const std::string &location = "");
  TestApp &wait_for_results_screen(float timeout_sec = 10.0f,
                                   const std::string &location = "");
  TestApp &expect_battle_not_tie(const std::string &location = "");
  TestApp &expect_battle_has_outcomes(const std::string &location = "");
  TestApp &expect_true(bool value, const std::string &description,
                       const std::string &location = "");
  TestApp &expect_false(bool value, const std::string &description,
                        const std::string &location = "");

  // Set bonus and synergy validation helpers
  TestApp &expect_synergy_count(CuisineTagType cuisine, int expected_count,
                                DishBattleState::TeamSide team,
                                const std::string &location = "");
  TestApp &expect_modifier(afterhours::EntityID dish_id, int expected_zing,
                           int expected_body, const std::string &location = "");

  // State inspection helpers
  TestApp &enable_state_inspection(const std::set<std::string> &flags = {
                                       "entities"});
  TestApp &disable_state_inspection();
  TestApp &clear_inspection_history();

  // Battle state helpers
  TestApp &clear_battle_dishes();

  // Kill the test server and wait for NetworkSystem to detect the failure
  TestApp &kill_server();

  // Force NetworkSystem to perform an immediate health check (for testing)
  TestApp &force_network_check();

private:
  afterhours::Entity *find_clickable_with(const std::string &label);
  void click_clickable(afterhours::Entity &entity);
  void setup_wait_state(WaitState::Type type, float timeout_sec,
                        const std::string &location = "");
  void pump_for_seconds(float seconds);
  bool has_onserve_completed();
  bool step_delay(); // Delay between steps when in non-headless mode. Returns
                     // true if delay was set up.
};

template <typename T>
TestApp &TestApp::expect_dish_has_component(afterhours::EntityID dish_id,
                                            const std::string &location) {
  auto *entity = find_entity_by_id(dish_id);
  if (!entity) {
    fail("Dish entity not found: " + std::to_string(dish_id), location);
  }
  if (!entity->has<T>()) {
    fail("Dish does not have expected component", location);
  }
  return *this;
}

template <typename Container>
TestApp &TestApp::expect_not_empty(const Container &collection,
                                   const std::string &description,
                                   const std::string &location) {
  if (collection.empty()) {
    fail("Expected " + description + " to not be empty, but it was empty",
         location);
  }
  return *this;
}

template <typename Container>
TestApp &TestApp::expect_empty(const Container &collection,
                               const std::string &description,
                               const std::string &location) {
  if (!collection.empty()) {
    fail("Expected " + description + " to be empty, but it had " +
             std::to_string(collection.size()) + " items",
         location);
  }
  return *this;
}

template <typename T>
TestApp &TestApp::expect_eq(const T &actual, const T &expected,
                            const std::string &description,
                            const std::string &location) {
  if (actual != expected) {
    std::stringstream ss;
    ss << "Expected " << description << " to equal " << expected << " but got "
       << actual;
    fail(ss.str(), location);
  }
  return *this;
}

template <typename T>
TestApp &TestApp::expect_entity_has_component(afterhours::EntityID entity_id,
                                              const std::string &location) {
  auto *entity = find_entity_by_id(entity_id);
  if (!entity) {
    fail("Entity not found: " + std::to_string(entity_id), location);
  }
  if (!entity->has<T>()) {
    fail("Entity does not have expected component", location);
  }
  return *this;
}

template <typename T>
TestApp &
TestApp::expect_singleton_has_component(afterhours::RefEntity &singleton_opt,
                                        const std::string &component_name,
                                        const std::string &location) {
  if (!singleton_opt.get().has<T>()) {
    fail("Singleton does not have " + component_name + " component", location);
  }
  return *this;
}
