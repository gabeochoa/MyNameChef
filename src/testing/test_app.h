#pragma once

#include "../components/battle_team_tags.h"
#include "../components/combat_stats.h"
#include "../components/deferred_flavor_mods.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/pending_combat_mods.h"
#include "../components/trigger_event.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../query.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <chrono>
#include <optional>
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

  afterhours::EntityID commit();
};

struct TestApp {
  std::string current_test_name;
  std::string failure_message;
  std::string failure_location;
  bool game_launched = false; // Track if launch_game() has been called

  // State for non-blocking waits in main game loop
  struct WaitState {
    enum Type { None, Screen, UI, FrameDelay };
    Type type = None;
    GameStateManager::Screen target_screen;
    std::string target_ui_label;
    float timeout_sec = 5.0f;
    std::chrono::steady_clock::time_point start_time;
    std::string location;
    int frame_delay_count = 0; // For frame-based delays
  } wait_state;

  TestApp() = default;

  void set_test_name(const std::string &name) { current_test_name = name; }

  void fail(const std::string &message, const std::string &location = "");

  // Check wait conditions (call this from TestSystem each frame)
  bool check_wait_conditions();

  TestApp &launch_game();
  TestApp &click(const std::string &button_label);
  TestApp &navigate_to_shop();
  TestApp &navigate_to_battle();

  std::vector<TestDishInfo> read_player_inventory();
  std::vector<TestShopItemInfo> read_store_options();
  int read_wallet_gold();
  int read_player_health();
  int count_active_player_dishes();
  int count_active_opponent_dishes();
  
  // Functions to manually manipulate game state for testing
  // These bypass normal game logic to set up specific test scenarios
  TestApp &set_wallet_gold(int gold, const std::string &location = "");
  TestApp &create_inventory_item(DishType type, int slot);
  GameStateManager::Screen read_current_screen();

  TestApp &wait_for_ui_exists(const std::string &label,
                              float timeout_sec = 5.0f,
                              const std::string &location = "");
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

  TestApp &wait_for_screen(GameStateManager::Screen screen,
                           float timeout_sec = 5.0f);
  TestApp &wait_for_frames(int frames);
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

private:
  afterhours::Entity *find_entity_by_id(afterhours::EntityID id);
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
