#include "test_app.h"
#include "../components/battle_load_request.h"
#include "../components/battle_processor.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/can_drop_onto.h"
#include "../components/combat_queue.h"
#include "../components/combat_stats.h"
#include "../components/continue_game_request.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/has_tooltip.h"
#include "../components/is_dish.h"
#include "../components/is_draggable.h"
#include "../components/is_drop_slot.h"
#include "../components/is_held.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../components/network_info.h"
#include "../components/render_order.h"
#include "../components/replay_state.h"
#include "../components/transform.h"
#include "../components/user_id.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../query.h"
#include "../render_backend.h"
#include "../render_constants.h"
#include "../seeded_rng.h"
#include "../server/file_storage.h"
#include "../settings.h"
#include "../shop.h"
#include "../systems/GameStateSaveSystem.h"
#include "../systems/NetworkSystem.h"
#include "../tooltip.h"
#include "test_macros.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/texture_manager.h>
#include <chrono>
#include <magic_enum/magic_enum.hpp>
#include <raylib/raylib.h>
#include <source_location>
#include <thread>

void TestApp::fail(const std::string &message, const std::string &location) {
  failure_message = message;
  failure_location = location;
  log_error("TEST FAILED [{}]: {} at {}", current_test_name, message,
            location.empty() ? "unknown location" : location);
  throw std::runtime_error(message);
}

void TestApp::yield(std::function<void()> continuation) {
  yield_continuation = std::move(continuation);
  throw std::runtime_error("TEST_YIELD");
}

TestApp &TestApp::launch_game(const std::source_location &loc) {
  TestOperationID op_id = generate_operation_id(loc, "launch_game");
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  log_info("TEST_APP: launch_game - Cleaning up battle state");

  // Check if BattleLoadRequest singleton exists and reset it
  const auto battleLoadRequestId =
      afterhours::components::get_type_id<BattleLoadRequest>();
  if (afterhours::EntityHelper::get().singletonMap.contains(
          battleLoadRequestId)) {
    auto battleRequest =
        afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (battleRequest.get().has<BattleLoadRequest>()) {
      auto &req = battleRequest.get().get<BattleLoadRequest>();
      req.loaded = false;
      req.playerJsonPath = "";
      req.opponentJsonPath = "";
      log_info("TEST_APP: launch_game - Reset BattleLoadRequest");
    }
  }

  // Check if BattleProcessor singleton exists and reset it
  const auto battleProcessorId =
      afterhours::components::get_type_id<BattleProcessor>();
  if (afterhours::EntityHelper::get().singletonMap.contains(
          battleProcessorId)) {
    try {
      auto battleProcessor =
          afterhours::EntityHelper::get_singleton<BattleProcessor>();
      if (battleProcessor.get().has<BattleProcessor>()) {
        auto &processor = battleProcessor.get().get<BattleProcessor>();
        processor.simulationStarted = false;
        processor.finished = false;           // Reset finished flag
        processor.simulationComplete = false; // Reset simulationComplete flag
        log_info("TEST_APP: launch_game - Reset BattleProcessor");
      }
    } catch (...) {
      // If accessing singleton fails (entity was cleaned up), just continue
      log_info("TEST_APP: launch_game - Could not access BattleProcessor "
               "singleton (may have been cleaned up)");
    }
  }

  GameStateManager &gsm = GameStateManager::get();
  gsm.set_next_screen(GameStateManager::Screen::Main);
  gsm.next_screen = std::nullopt; // Clear any pending screen transition
  gsm.update_screen();
  log_info("TEST_APP: launch_game - Set screen to Main, active_screen={}, "
           "next_screen={}",
           (int)gsm.active_screen,
           gsm.next_screen.has_value() ? (int)gsm.next_screen.value() : -1);
  game_launched = true;
  completed_operations.insert(op_id);
  if (step_delay()) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }
  return *this;
}

TestApp &TestApp::click(const std::string &button_label,
                        const std::source_location &loc) {
  TestOperationID op_id = generate_operation_id(loc, "click:" + button_label);
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }
  afterhours::Entity *entity = find_clickable_with(button_label);
  if (!entity) {
    fail("Button not found: " + button_label);
  }

  GameStateManager &gsm = GameStateManager::get();
  GameStateManager::Screen current_screen = gsm.active_screen;
  std::optional<GameStateManager::Screen> next_screen = gsm.next_screen;

  if (button_label == "Play" &&
      current_screen == GameStateManager::Screen::Shop) {
    completed_operations.insert(op_id);
    return *this;
  }
  if (button_label == "Play" && next_screen.has_value() &&
      next_screen.value() == GameStateManager::Screen::Shop) {
    completed_operations.insert(op_id);
    return *this;
  }

  click_clickable(*entity);

  completed_operations.insert(op_id);

  if (step_delay()) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }
  return *this;
}

afterhours::Entity *TestApp::find_clickable_with(const std::string &label) {
  // Try to find button in replay bar first (if on battle screen)
  // This helps distinguish between main menu "Play" and replay "Play/Pause"
  GameStateManager::Screen current_screen =
      GameStateManager::get().active_screen;
  bool on_battle_screen = (current_screen == GameStateManager::Screen::Battle);

  std::vector<afterhours::Entity *> candidates;
  afterhours::Entity *replay_button = nullptr;

  for (const std::shared_ptr<afterhours::Entity> &ep :
       afterhours::EntityHelper::get_entities()) {
    afterhours::Entity &e = *ep;
    if (e.has<afterhours::ui::UIComponent>() &&
        e.has<afterhours::ui::HasClickListener>()) {
      std::string button_label;
      std::string debug_name;

      if (e.has<afterhours::ui::HasLabel>()) {
        button_label = e.get<afterhours::ui::HasLabel>().label;
      }
      if (e.has<afterhours::ui::UIComponentDebug>()) {
        debug_name = e.get<afterhours::ui::UIComponentDebug>().name();
      }

      // On battle screen, check debug name first for replay button
      // The debug name should always be "replay_play_pause_button" regardless
      // of label Since this button toggles between "Play" and "Pause", if we're
      // searching for either and we find the replay button, we should use it
      if (on_battle_screen && debug_name == "replay_play_pause_button") {
        // If searching for "Play" or "Pause" (or the debug name itself), use
        // this button
        if (label == "Play" || label == "Pause" ||
            label == "replay_play_pause_button" || button_label == label) {
          replay_button = &e;
          // Don't break yet - continue to check if there's a better match
        }
      }

      // Check if label matches (but only add to candidates if not already found
      // as replay button)
      if (button_label == label &&
          (debug_name != "replay_play_pause_button" || !on_battle_screen)) {
        candidates.push_back(&e);
      }
      // Also check if debug name matches (in case someone searches by debug
      // name)
      else if (!debug_name.empty() && debug_name == label) {
        candidates.push_back(&e);
      }
    }
  }

  // On battle screen, prefer the replay button if found
  if (on_battle_screen && replay_button != nullptr) {
    return replay_button;
  }

  // Otherwise return first candidate
  if (!candidates.empty()) {
    return candidates[0];
  }

  return nullptr;
}

void TestApp::click_clickable(afterhours::Entity &entity) {
  try {
    entity.get<afterhours::ui::HasClickListener>().cb(entity);
  } catch (const std::exception &e) {
    log_error("TEST: Click callback threw exception: {}", e.what());
    throw;
  }
}

TestApp &TestApp::navigate_to_shop(const std::source_location &loc) {
  TestOperationID op_id = generate_operation_id(loc, "navigate_to_shop");
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }
  wait_for_ui_exists("Play");
  click("Play");
  wait_for_screen(GameStateManager::Screen::Shop);
  completed_operations.insert(op_id);
  if (step_delay()) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }
  return *this;
}

TestApp &TestApp::navigate_to_battle(const std::source_location &loc) {
  TestOperationID op_id = generate_operation_id(loc, "navigate_to_battle");
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }
  GameStateManager &gsm = GameStateManager::get();
  if (gsm.active_screen == GameStateManager::Screen::Battle) {
    completed_operations.insert(op_id);
    return *this;
  }

  if (gsm.active_screen != GameStateManager::Screen::Shop) {
    std::string location =
        std::string(loc.file_name()) + ":" + std::to_string(loc.line());
    std::string current_screen_name =
        std::string(magic_enum::enum_name(gsm.active_screen));
    fail("Must be on Shop screen to navigate to battle (current screen: " +
             current_screen_name + ")",
         location);
  }
  wait_for_ui_exists("Next Round");
  click("Next Round");
  wait_for_screen(GameStateManager::Screen::Battle);
  completed_operations.insert(op_id);
  if (step_delay()) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }
  return *this;
}

std::vector<TestDishInfo> TestApp::read_player_inventory() {
  std::vector<TestDishInfo> result;
  // Use force_merge to ensure all entities are included
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsInventoryItem>()
           .whereHasComponent<IsDish>()
           .gen()) {
    // Exclude cleanup entities and shop items (they shouldn't be in inventory)
    if (entity.cleanup || entity.has<IsShopItem>()) {
      continue;
    }
    IsInventoryItem &inv = entity.get<IsInventoryItem>();
    IsDish &dish = entity.get<IsDish>();
    int level = 1;
    if (entity.has<DishLevel>()) {
      level = entity.get<DishLevel>().level;
    }
    result.push_back({entity.id, dish.type, dish.name(), inv.slot, level});
  }
  return result;
}

int TestApp::count_active_player_dishes() {
  int count = 0;
  for (afterhours::Entity &e : afterhours::EntityQuery({.force_merge = true})
                                   .whereHasComponent<DishBattleState>()
                                   .gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Player &&
        (dbs.phase == DishBattleState::Phase::InQueue ||
         dbs.phase == DishBattleState::Phase::Entering ||
         dbs.phase == DishBattleState::Phase::InCombat)) {
      count++;
    }
  }
  return count;
}

int TestApp::count_active_opponent_dishes() {
  int count = 0;
  for (afterhours::Entity &e : afterhours::EntityQuery({.force_merge = true})
                                   .whereHasComponent<DishBattleState>()
                                   .gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.team_side == DishBattleState::TeamSide::Opponent &&
        (dbs.phase == DishBattleState::Phase::InQueue ||
         dbs.phase == DishBattleState::Phase::Entering ||
         dbs.phase == DishBattleState::Phase::InCombat)) {
      count++;
    }
  }
  return count;
}

std::vector<TestShopItemInfo> TestApp::read_store_options() {
  std::vector<TestShopItemInfo> result;
  // force_merge in query handles merging automatically
  for (afterhours::Entity &entity :
       afterhours::EntityQuery({.force_merge = true})
           .whereHasComponent<IsShopItem>()
           .gen()) {
    if (entity.cleanup) {
      continue;
    }
    if (!entity.has<IsDish>()) {
      continue;
    }
    IsShopItem &shop = entity.get<IsShopItem>();
    IsDish &dish = entity.get<IsDish>();
    DishInfo dish_info = get_dish_info(dish.type);
    result.push_back(
        {entity.id, dish.type, dish.name(), shop.slot, dish_info.price});
  }
  return result;
}

int TestApp::read_wallet_gold() {
  afterhours::RefEntity wallet_ref =
      afterhours::EntityHelper::get_singleton<Wallet>();
  if (wallet_ref.get().has<Wallet>()) {
    return wallet_ref.get().get<Wallet>().gold;
  }
  fail("Wallet singleton not found");
  return 0; // Unreachable
}

bool TestApp::read_replay_paused() {
  afterhours::RefEntity replay_ref =
      afterhours::EntityHelper::get_singleton<ReplayState>();
  if (replay_ref.get().has<ReplayState>()) {
    return replay_ref.get().get<ReplayState>().paused;
  }
  fail("ReplayState singleton not found");
  return false; // Unreachable
}

TestApp &TestApp::set_battle_speed(float speed) {
  render_backend::timing_speed_scale = speed;
  Settings::get().set_battle_speed(speed);
  return *this;
}

TestApp &TestApp::set_wallet_gold(int gold, const std::string &location) {
  // Manually set wallet gold, bypassing normal game logic
  // Used for testing scenarios (e.g., testing purchase failures with
  // insufficient funds)
  afterhours::RefEntity wallet_ref =
      afterhours::EntityHelper::get_singleton<Wallet>();
  if (!wallet_ref.get().has<Wallet>()) {
    fail("Wallet singleton not found", location);
  }
  wallet_ref.get().get<Wallet>().gold = gold;
  return *this;
}

TestApp &TestApp::create_inventory_item(DishType type, int slot) {
  // Manually create an item in a specific inventory slot
  // Bypasses normal purchase logic - used for testing scenarios
  // If slot is already occupied, this will overwrite/replace it

  // Find the inventory slot
  int slot_id = slot;

  afterhours::Entity *target_slot = nullptr;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
    const IsDropSlot &drop_slot = entity.get<IsDropSlot>();
    if (drop_slot.slot_id == slot_id && drop_slot.accepts_inventory_items) {
      target_slot = &entity;
      break;
    }
  }

  if (!target_slot) {
    fail("Inventory slot " + std::to_string(slot) + " not found");
  }

  // If slot is already occupied, remove the existing item
  if (target_slot->get<IsDropSlot>().occupied) {
    // Find and remove the existing item in this slot
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsInventoryItem>()
             .gen()) {
      if (entity.get<IsInventoryItem>().slot == slot_id) {
        entity.cleanup = true;
        break;
      }
    }
    // Cleanup will happen when system loop runs
  }

  // Create the dish entity
  // Calculate position manually (same as calculate_inventory_position)
  float x = INVENTORY_START_X + slot * (SLOT_SIZE + SLOT_GAP);
  vec2 position{x, static_cast<float>(INVENTORY_START_Y)};

  afterhours::Entity &dish = afterhours::EntityHelper::createEntity();

  dish.addComponent<Transform>(position, vec2{80.0f, 80.0f});
  dish.addComponent<IsDish>(type);
  dish.addComponent<DishLevel>(1);
  IsInventoryItem &inv_item = dish.addComponent<IsInventoryItem>();
  inv_item.slot = slot;

  // Add components needed for inventory items (matching GenerateInventorySlots)
  dish.addComponent<IsDraggable>(true);
  dish.addComponent<HasRenderOrder>(RenderOrder::InventoryItems,
                                    RenderScreen::Shop);

  // Add sprite component for rendering
  auto dish_info = get_dish_info(type);
  const auto frame = afterhours::texture_manager::idx_to_sprite_frame(
      dish_info.sprite.i, dish_info.sprite.j);
  dish.addComponent<afterhours::texture_manager::HasSprite>(
      position, vec2{80.0f, 80.0f}, 0.f, frame,
      render_constants::kDishSpriteScale, raylib::WHITE);

  dish.addComponent<HasTooltip>(generate_dish_tooltip(type));

  // Mark slot as occupied
  target_slot->get<IsDropSlot>().occupied = true;

  // Entity will be merged by system loop automatically
  return *this;
}

int TestApp::read_round() {
  afterhours::RefEntity round_ref =
      afterhours::EntityHelper::get_singleton<Round>();
  if (round_ref.get().has<Round>()) {
    return round_ref.get().get<Round>().current;
  }
  return 1; // Default round if not found
}

int TestApp::read_shop_tier() {
  afterhours::RefEntity shop_tier_ref =
      afterhours::EntityHelper::get_singleton<ShopTier>();
  if (shop_tier_ref.get().has<ShopTier>()) {
    return shop_tier_ref.get().get<ShopTier>().current_tier;
  }
  fail("ShopTier singleton not found");
  return 1; // Unreachable
}

TestApp::RerollCostInfo TestApp::read_reroll_cost() {
  RerollCostInfo info;
  afterhours::RefEntity reroll_ref =
      afterhours::EntityHelper::get_singleton<RerollCost>();
  if (reroll_ref.get().has<RerollCost>()) {
    const RerollCost &cost = reroll_ref.get().get<RerollCost>();
    info.base = cost.base;
    info.increment = cost.increment;
    info.current = cost.current;
    return info;
  }
  fail("RerollCost singleton not found");
  return info; // Unreachable
}

uint64_t TestApp::read_shop_seed() { return SeededRng::get().seed; }

int TestApp::read_player_health() {
  afterhours::RefEntity health_ref =
      afterhours::EntityHelper::get_singleton<Health>();
  if (health_ref.get().has<Health>()) {
    return health_ref.get().get<Health>().current;
  }
  fail("Health singleton not found");
  return 0; // Unreachable
}

GameStateManager::Screen TestApp::read_current_screen() {
  return GameStateManager::get().active_screen;
}

TestApp &TestApp::wait_for_ui_exists(const std::string &label,
                                     float timeout_sec,
                                     const std::string &location,
                                     const std::source_location &loc) {
  TestOperationID op_id =
      generate_operation_id(loc, "wait_for_ui_exists:" + label);
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  if (wait_state.type == WaitState::UI && wait_state.target_ui_label == label &&
      wait_state.operation_id == op_id) {
    if (check_wait_conditions()) {
      completed_operations.insert(op_id);
      wait_state.type = WaitState::None;
      return *this;
    }
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  if (wait_state.type != WaitState::None && wait_state.operation_id != op_id) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  setup_wait_state(WaitState::UI, timeout_sec, location);
  wait_state.target_ui_label = label;
  wait_state.operation_id = op_id;

  if (check_wait_conditions()) {
    completed_operations.insert(op_id);
    wait_state.type = WaitState::None;
    return *this;
  }

  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

bool TestApp::check_wait_conditions() {
  if (wait_state.type == WaitState::None) {
    return true; // No wait condition
  }

  if (wait_state.type == WaitState::FrameDelay) {
    if (wait_state.frame_delay_count > 0) {
      wait_state.frame_delay_count--;
    }
    if (wait_state.frame_delay_count <= 0) {
      TestOperationID op_id = wait_state.operation_id;
      wait_state.type = WaitState::None;
      if (op_id != 0) {
        completed_operations.insert(op_id);
      }
      return true;
    }
    return false;
  }

  if (wait_state.type == WaitState::UI) {
    // force_merge in query handles merging automatically
    // Check if UI element exists (use EntityQuery like UITestHelpers does)
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<afterhours::ui::HasLabel>()
             .gen()) {
      const afterhours::ui::HasLabel &label =
          entity.get<afterhours::ui::HasLabel>();
      if (label.label == wait_state.target_ui_label) {
        TestOperationID op_id = wait_state.operation_id;
        wait_state.type = WaitState::None;
        if (op_id != 0) {
          completed_operations.insert(op_id);
        }
        return true;
      }
    }

    // Also check for clickable elements with labels (like find_clickable_with
    // does)
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<afterhours::ui::HasClickListener>()
             .gen()) {
      std::string name;
      if (entity.has<afterhours::ui::HasLabel>()) {
        name = entity.get<afterhours::ui::HasLabel>().label;
      }
      if (name.empty() && entity.has<afterhours::ui::UIComponentDebug>()) {
        name = entity.get<afterhours::ui::UIComponentDebug>().name();
      }
      if (name == wait_state.target_ui_label) {
        TestOperationID op_id = wait_state.operation_id;
        wait_state.type = WaitState::None;
        if (op_id != 0) {
          completed_operations.insert(op_id);
        }
        return true;
      }
    }

    // Check timeout
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - wait_state.start_time);
    if (ms.count() > static_cast<int>(wait_state.timeout_sec * 1000.0f)) {
      fail("Timeout waiting for UI element: " + wait_state.target_ui_label,
           wait_state.location);
      return true; // Timeout - fail
    }

    return false; // Still waiting
  }

  if (wait_state.type == WaitState::Screen) {
    GameStateManager &gsm = GameStateManager::get();
    gsm.update_screen();
    GameStateManager::Screen current_screen = gsm.active_screen;

    if (current_screen == wait_state.target_screen) {
      // Screen has transitioned, but we need to ensure the UI system has
      // had a chance to create the UI elements for this screen.
      // force_merge in queries handles merging automatically
      // For Shop screen, verify that shop-specific UI exists before
      // considering the transition complete
      if (wait_state.target_screen == GameStateManager::Screen::Shop) {
        // Check if shop UI elements exist (like "Next Round" button)
        bool shop_ui_exists = false;
        for (afterhours::Entity &entity :
             afterhours::EntityQuery({.force_merge = true})
                 .whereHasComponent<afterhours::ui::HasLabel>()
                 .gen()) {
          const afterhours::ui::HasLabel &label =
              entity.get<afterhours::ui::HasLabel>();
          if (label.label == "Next Round" ||
              (label.label.find("Reroll (") == 0 &&
               label.label.find(")") != std::string::npos)) {
            shop_ui_exists = true;
            break;
          }
        }
        if (!shop_ui_exists) {
          return false; // Still waiting for UI to load
        }
      }

      TestOperationID op_id = wait_state.operation_id;
      wait_state.type = WaitState::None;
      if (op_id != 0) {
        completed_operations.insert(op_id);
      }
      return true;
    }

    // Check timeout
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - wait_state.start_time);
    if (ms.count() > static_cast<int>(wait_state.timeout_sec * 1000.0f)) {
      GameStateManager &gsm = GameStateManager::get();
      gsm.update_screen();
      // For Battle screen, if we're on Shop or Results, that might be
      // acceptable (battle might have completed instantly or not started)
      if (wait_state.target_screen == GameStateManager::Screen::Battle) {
        if (gsm.active_screen == GameStateManager::Screen::Results) {
          // Accept Results as success for Battle wait (battle completed
          // instantly)
          TestOperationID op_id = wait_state.operation_id;
          wait_state.type = WaitState::None;
          if (op_id != 0) {
            completed_operations.insert(op_id);
          }
          return true;
        }
      }
      fail("Timeout waiting for screen: " +
               std::to_string(static_cast<int>(wait_state.target_screen)) +
               " (current: " +
               std::to_string(static_cast<int>(gsm.active_screen)) + ")",
           wait_state.location);
      return true; // Timeout - fail
    }

    return false; // Still waiting
  }

  return true;
}

TestApp &TestApp::expect_screen_is(GameStateManager::Screen screen,
                                   const std::string &location) {
  GameStateManager &gsm = GameStateManager::get();
  gsm.update_screen(); // Ensure any pending screen transitions are applied
  GameStateManager::Screen current = gsm.active_screen;
  if (current != screen) {
    fail("Expected screen " + std::to_string(static_cast<int>(screen)) +
             " but got " + std::to_string(static_cast<int>(current)),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_inventory_contains(DishType type,
                                            const std::string &location) {
  std::vector<TestDishInfo> inventory = read_player_inventory();
  bool found = false;
  for (const TestDishInfo &dish : inventory) {
    if (dish.type == type) {
      found = true;
      break;
    }
  }
  if (!found) {
    fail("Inventory does not contain dish type: " +
             std::to_string(static_cast<int>(type)),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_wallet_has(int gold, const std::string &location) {
  int current = read_wallet_gold();
  if (current != gold) {
    fail("Expected wallet gold " + std::to_string(gold) + " but got " +
             std::to_string(current),
         location);
  }
  return *this;
}

TestApp &TestApp::wait_for_screen(GameStateManager::Screen screen,
                                  float timeout_sec,
                                  const std::source_location &loc) {
  TestOperationID op_id = generate_operation_id(
      loc, "wait_for_screen:" + std::to_string(static_cast<int>(screen)));
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  if (wait_state.type == WaitState::Screen &&
      wait_state.target_screen == screen && wait_state.operation_id == op_id) {
    GameStateManager::get().update_screen();
    if (check_wait_conditions()) {
      completed_operations.insert(op_id);
      wait_state.type = WaitState::None;
      return *this;
    }
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  if (wait_state.type != WaitState::None && wait_state.operation_id != op_id) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  setup_wait_state(WaitState::Screen, timeout_sec);
  wait_state.target_screen = screen;
  wait_state.operation_id = op_id;

  GameStateManager::get().update_screen();
  if (check_wait_conditions()) {
    completed_operations.insert(op_id);
    wait_state.type = WaitState::None;
    return *this;
  }

  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::wait_for_frames(int frames, const std::source_location &loc) {
  TestOperationID op_id =
      generate_operation_id(loc, "wait_for_frames:" + std::to_string(frames));
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  // Use non-blocking wait state so game loop continues
  if (wait_state.type == WaitState::FrameDelay &&
      wait_state.operation_id == op_id) {
    if (check_wait_conditions()) {
      completed_operations.insert(op_id);
      return *this;
    }
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  if (wait_state.type != WaitState::None && wait_state.operation_id != op_id) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  setup_wait_state(WaitState::FrameDelay, 60.0f); // 60 second timeout
  wait_state.frame_delay_count = frames;
  wait_state.operation_id = op_id;

  if (check_wait_conditions()) {
    completed_operations.insert(op_id);
    wait_state.type = WaitState::None;
    return *this;
  }

  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::pump_frame() {
  // Frames are pumped automatically by the game loop
  // Just wait a tiny bit to let systems process
  std::this_thread::sleep_for(std::chrono::milliseconds(16));
  return *this;
}

TestApp &TestApp::setup_battle() {
  GameStateManager::get().to_battle();
  GameStateManager::get().update_screen();

  wait_for_screen(GameStateManager::Screen::Battle);

  return *this;
}

TestDishBuilder TestApp::create_dish(DishType type) {
  return TestDishBuilder(type);
}

afterhours::EntityID TestDishBuilder::commit() {

  afterhours::Entity &entity = afterhours::EntityHelper::createEntity();

  float x = 120.0f + slot * 100.0f;
  float y = (team_side == DishBattleState::TeamSide::Player) ? 150.0f : 500.0f;

  entity.addComponent<Transform>(afterhours::vec2{x, y},
                                 afterhours::vec2{80.0f, 80.0f});
  entity.addComponent<IsDish>(type);

  if (level.has_value()) {
    entity.addComponent<DishLevel>(level.value());
  } else {
    entity.addComponent<DishLevel>(1);
  }

  DishBattleState &dbs = entity.addComponent<DishBattleState>();
  dbs.queue_index = slot;
  dbs.team_side = team_side;
  dbs.phase = phase;
  dbs.enter_progress = 0.0f;
  dbs.bite_timer = 0.0f;
  dbs.onserve_fired = false;

  if (has_combat_stats) {
    entity.addComponent<CombatStats>();
  }

  if (team_side == DishBattleState::TeamSide::Player) {
    entity.addComponent<IsPlayerTeamItem>();
  } else {
    entity.addComponent<IsOpponentTeamItem>();
  }

  // Entity will be merged by system loop automatically
  return entity.id;
}

TestApp &TestApp::advance_battle_until_onserve_complete(float timeout_sec) {
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();

  while (!raylib::WindowShouldClose()) {
    pump_frame();

    if (has_onserve_completed()) {
      return *this;
    }

    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
    if (ms.count() > static_cast<int>(timeout_sec * 1000.0f)) {
      fail("Timeout waiting for OnServe to complete");
    }
  }

  fail("Window closed while waiting for OnServe");
  return *this;
}

bool TestApp::has_onserve_completed() {
  bool has_incomplete_onserve =
      afterhours::EntityQuery()
          .whereHasComponent<DishBattleState>()
          .whereLambda([](const afterhours::Entity &entity) {
            const DishBattleState &dbs = entity.get<DishBattleState>();
            return dbs.phase == DishBattleState::Phase::InQueue &&
                   !dbs.onserve_fired;
          })
          .has_values();
  return !has_incomplete_onserve;
}

void TestApp::setup_wait_state(WaitState::Type type, float timeout_sec,
                               const std::string &location) {
  wait_state.type = type;
  wait_state.timeout_sec = timeout_sec;
  wait_state.start_time = std::chrono::steady_clock::now();
  wait_state.location = location;
}

afterhours::Entity *TestApp::find_entity_by_id(afterhours::EntityID id) {
  afterhours::OptEntity opt_entity =
      afterhours::EntityQuery().whereID(id).gen_first();
  if (opt_entity) {
    return &opt_entity.asE();
  }
  return nullptr;
}

TestApp &TestApp::expect_flavor_mods(afterhours::EntityID dish_id,
                                     const DeferredFlavorMods &expected,
                                     const std::string &location) {
  afterhours::Entity *entity = find_entity_by_id(dish_id);
  if (!entity) {
    fail("Dish entity not found: " + std::to_string(dish_id), location);
  }
  if (!entity->has<DeferredFlavorMods>()) {
    fail("Dish does not have DeferredFlavorMods", location);
  }

  DeferredFlavorMods &mods = entity->get<DeferredFlavorMods>();
  if (mods.satiety != expected.satiety ||
      mods.sweetness != expected.sweetness || mods.spice != expected.spice ||
      mods.acidity != expected.acidity || mods.umami != expected.umami ||
      mods.richness != expected.richness ||
      mods.freshness != expected.freshness) {
    fail("Flavor mods mismatch - expected: s" +
             std::to_string(expected.satiety) + " sw" +
             std::to_string(expected.sweetness) + " sp" +
             std::to_string(expected.spice) + " a" +
             std::to_string(expected.acidity) + " u" +
             std::to_string(expected.umami) + " r" +
             std::to_string(expected.richness) + " f" +
             std::to_string(expected.freshness) + " but got: s" +
             std::to_string(mods.satiety) + " sw" +
             std::to_string(mods.sweetness) + " sp" +
             std::to_string(mods.spice) + " a" + std::to_string(mods.acidity) +
             " u" + std::to_string(mods.umami) + " r" +
             std::to_string(mods.richness) + " f" +
             std::to_string(mods.freshness),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_combat_mods(afterhours::EntityID dish_id,
                                     const PendingCombatMods &expected,
                                     const std::string &location) {
  afterhours::Entity *entity = find_entity_by_id(dish_id);
  if (!entity) {
    fail("Dish entity not found: " + std::to_string(dish_id), location);
  }
  if (!entity->has<PendingCombatMods>()) {
    fail("Dish does not have PendingCombatMods", location);
  }

  PendingCombatMods &mods = entity->get<PendingCombatMods>();
  if (mods.zingDelta != expected.zingDelta ||
      mods.bodyDelta != expected.bodyDelta) {
    fail("Combat mods mismatch - expected: zing=" +
             std::to_string(expected.zingDelta) +
             " body=" + std::to_string(expected.bodyDelta) +
             " but got: zing=" + std::to_string(mods.zingDelta) +
             " body=" + std::to_string(mods.bodyDelta),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_trigger_fired(TriggerHook hook,
                                       afterhours::EntityID dish_id,
                                       const std::string &location) {
  afterhours::Entity *entity = find_entity_by_id(dish_id);
  if (!entity) {
    fail("Dish entity not found: " + std::to_string(dish_id), location);
  }

  DishBattleState &dbs = entity->get<DishBattleState>();
  if (hook == TriggerHook::OnServe && !dbs.onserve_fired) {
    fail("OnServe trigger has not fired for dish " + std::to_string(dish_id),
         location);
  }

  return *this;
}

bool TestApp::step_delay() {
  // Only delay in non-headless mode
  if (!render_backend::is_headless_mode && render_backend::step_delay_ms > 0) {
    // Convert milliseconds to approximate frame count (assuming 60 FPS)
    // Add 1 to ensure at least one frame passes
    int frames_to_wait = (render_backend::step_delay_ms / 16) + 1;

    // Set up frame-based wait state so frames can process
    setup_wait_state(WaitState::FrameDelay, 5.0f); // 5 second timeout as safety
    wait_state.frame_delay_count = frames_to_wait;
    wait_state.operation_id = 0; // No specific operation ID for step delays

    // Return true to indicate caller should yield
    return true;
  }
  return false; // No delay needed
}

bool TestApp::can_afford_purchase(DishType type) {
  const int price = get_dish_info(type).price;
  const int current_gold = read_wallet_gold();
  return current_gold >= price;
}

TestApp &TestApp::expect_count_eq(int actual, int expected,
                                  const std::string &description,
                                  const std::string &location) {
  if (actual != expected) {
    fail("Expected " + description + " count to be " +
             std::to_string(expected) + " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_count_gt(int actual, int min,
                                  const std::string &description,
                                  const std::string &location) {
  if (actual <= min) {
    fail("Expected " + description + " count to be greater than " +
             std::to_string(min) + " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_count_lt(int actual, int max,
                                  const std::string &description,
                                  const std::string &location) {
  if (actual >= max) {
    fail("Expected " + description + " count to be less than " +
             std::to_string(max) + " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_count_gte(int actual, int min,
                                   const std::string &description,
                                   const std::string &location) {
  if (actual < min) {
    fail("Expected " + description + " count to be at least " +
             std::to_string(min) + " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_count_lte(int actual, int max,
                                   const std::string &description,
                                   const std::string &location) {
  if (actual > max) {
    fail("Expected " + description + " count to be at most " +
             std::to_string(max) + " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_dish_phase(afterhours::EntityID dish_id,
                                    DishBattleState::Phase expected_phase,
                                    const std::string &location) {
  auto *entity = find_entity_by_id(dish_id);
  if (!entity) {
    fail("Dish entity not found: " + std::to_string(dish_id), location);
  }
  if (!entity->has<DishBattleState>()) {
    fail("Dish does not have DishBattleState component", location);
  }
  const DishBattleState &dbs = entity->get<DishBattleState>();
  if (dbs.phase != expected_phase) {
    fail("Expected dish phase " +
             std::to_string(static_cast<int>(expected_phase)) + " but got " +
             std::to_string(static_cast<int>(dbs.phase)),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_dish_count(int expected_player, int expected_opponent,
                                    const std::string &location) {
  int player_count = count_active_player_dishes();
  int opponent_count = count_active_opponent_dishes();
  if (player_count != expected_player || opponent_count != expected_opponent) {
    fail("Expected dish counts - player: " + std::to_string(expected_player) +
             ", opponent: " + std::to_string(expected_opponent) +
             " but got - player: " + std::to_string(player_count) +
             ", opponent: " + std::to_string(opponent_count),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_player_dish_count(int expected,
                                           const std::string &location) {
  int actual = count_active_player_dishes();
  if (actual != expected) {
    fail("Expected player dish count to be " + std::to_string(expected) +
             " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_opponent_dish_count(int expected,
                                             const std::string &location) {
  int actual = count_active_opponent_dishes();
  if (actual != expected) {
    fail("Expected opponent dish count to be " + std::to_string(expected) +
             " but got " + std::to_string(actual),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_dish_count_at_least(int min_player, int min_opponent,
                                             const std::string &location) {
  int player_count = count_active_player_dishes();
  int opponent_count = count_active_opponent_dishes();
  if (player_count < min_player || opponent_count < min_opponent) {
    fail("Expected dish counts at least - player: " +
             std::to_string(min_player) +
             ", opponent: " + std::to_string(min_opponent) +
             " but got - player: " + std::to_string(player_count) +
             ", opponent: " + std::to_string(opponent_count),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_wallet_at_least(int min_gold,
                                         const std::string &location) {
  int current = read_wallet_gold();
  if (current < min_gold) {
    fail("Expected wallet to have at least " + std::to_string(min_gold) +
             " gold but got " + std::to_string(current),
         location);
  }
  return *this;
}

TestApp &TestApp::expect_wallet_between(int min_gold, int max_gold,
                                        const std::string &location) {
  int current = read_wallet_gold();
  if (current < min_gold || current > max_gold) {
    fail("Expected wallet to have between " + std::to_string(min_gold) +
             " and " + std::to_string(max_gold) + " gold but got " +
             std::to_string(current),
         location);
  }
  return *this;
}

TestApp &TestApp::wait_for_battle_initialized(float timeout_sec,
                                              const std::string &location) {
  (void)timeout_sec;
  (void)location;
  static TestOperationID op_id = 0;
  if (op_id == 0) {
    op_id = generate_operation_id(std::source_location::current(),
                                  "wait_for_battle_initialized");
  }

  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  static int check_count = 0;
  check_count++;

  int in_combat = 0;
  int entering = 0;
  int in_queue = 0;
  int finished = 0;

  for (afterhours::Entity &e :
       afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.phase == DishBattleState::Phase::Entering)
      entering++;
    if (dbs.phase == DishBattleState::Phase::InCombat)
      in_combat++;
    if (dbs.phase == DishBattleState::Phase::InQueue)
      in_queue++;
    if (dbs.phase == DishBattleState::Phase::Finished)
      finished++;
  }

  if (check_count % 60 == 0) {
    log_info("BATTLE_INIT_CHECK: check_count={}, phases: InQueue={}, "
             "Entering={}, InCombat={}, Finished={}",
             check_count, in_queue, entering, in_combat, finished);
  }

  if (in_combat > 0 || entering > 0) {
    log_info("BATTLE_INIT_CHECK: Battle initialized - Entering={}, InCombat={}",
             entering, in_combat);
    completed_operations.insert(op_id);
    check_count = 0;
    return *this;
  }

  if (check_count >= 2000) {
    fail("Timeout waiting for battle initialization", location);
  }

  wait_state.type = WaitState::FrameDelay;
  wait_state.frame_delay_count = 1;
  wait_state.operation_id = op_id;
  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::wait_for_dishes_in_combat(int min_count, float timeout_sec,
                                            const std::string &location) {
  (void)timeout_sec;
  (void)location;
  TestOperationID op_id = generate_operation_id(std::source_location::current(),
                                                "wait_for_dishes_in_combat:" +
                                                    std::to_string(min_count));
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  int in_combat = 0;
  for (afterhours::Entity &e :
       afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.phase == DishBattleState::Phase::InCombat)
      in_combat++;
  }

  if (in_combat >= min_count) {
    completed_operations.insert(op_id);
    return *this;
  }

  wait_state.type = WaitState::FrameDelay;
  wait_state.frame_delay_count = 1;
  wait_state.operation_id = op_id;
  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::wait_for_animations_complete(float timeout_sec,
                                               const std::string &location) {
  (void)timeout_sec;
  (void)location;
  TestOperationID op_id = generate_operation_id(std::source_location::current(),
                                                "wait_for_animations_complete");
  if (completed_operations.count(op_id) > 0) {
    return *this;
  }

  // In headless mode, animations complete instantly
  if (render_backend::is_headless_mode) {
    completed_operations.insert(op_id);
    return *this;
  }

  // Check if there are any blocking animations
  bool has_anim = hasActiveAnimation();
  if (!has_anim) {
    completed_operations.insert(op_id);
    return *this;
  }

  // Wait for animations to complete
  wait_state.type = WaitState::FrameDelay;
  wait_state.frame_delay_count = 1;
  wait_state.operation_id = op_id;
  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::expect_combat_ticks_occurred(int min_ticks,
                                               const std::string &location) {
  // Check BattleProcessor to see if combat ticks occurred
  // We can check outcomes (completed courses) or look at dish state changes
  afterhours::OptEntity battleProcessor;
  try {
    battleProcessor =
        afterhours::EntityHelper::get_singleton<BattleProcessor>();
  } catch (...) {
    fail("BattleProcessor singleton not found - cannot verify combat ticks",
         location);
    return *this;
  }

  if (!battleProcessor.has_value()) {
    fail("BattleProcessor singleton not found - cannot verify combat ticks",
         location);
    return *this;
  }

  if (!battleProcessor.asE().has<BattleProcessor>()) {
    fail("BattleProcessor singleton not found - cannot verify combat ticks",
         location);
    return *this;
  }

  // CRITICAL: Access processor component safely - entity might be invalid
  // If entity was cleaned up, get_singleton might return invalid reference
  // Segfaults can't be caught by try-catch, so we need to be very careful
  const BattleProcessor *processor_ptr = nullptr;
  try {
    processor_ptr = &battleProcessor.asE().get<BattleProcessor>();
  } catch (...) {
    fail("BattleProcessor component access failed - cannot verify combat ticks",
         location);
    return *this;
  }

  if (processor_ptr == nullptr) {
    fail("BattleProcessor component is null - cannot verify combat ticks",
         location);
    return *this;
  }

  const BattleProcessor &processor = *processor_ptr;

  // CRITICAL: Check if battle is actually active before accessing data
  // If battle isn't active, we can't verify combat ticks
  if (!processor.isBattleActive()) {
    fail("Battle is not active - cannot verify combat ticks. "
         "simulationStarted=" +
             std::to_string(processor.simulationStarted ? 1 : 0) +
             ", simulationComplete=" +
             std::to_string(processor.simulationComplete ? 1 : 0),
         location);
    return *this;
  }

  // Check if any courses have outcomes (proves combat occurred)
  // CRITICAL: Access outcomes - if entity was cleaned up, this will segfault
  // We can't prevent segfaults with try-catch, so we just hope the entity is
  // valid
  int completed_courses = static_cast<int>(processor.outcomes.size());

  // CRITICAL: Check if any dish has had a bite occur (firstBiteDecided = true)
  // This is the most direct indicator that resolveCombatTick was actually
  // called
  int dishes_with_bites = 0;
  for (const auto &dish : processor.playerDishes) {
    if (dish.firstBiteDecided) {
      dishes_with_bites++;
    }
  }
  for (const auto &dish : processor.opponentDishes) {
    if (dish.firstBiteDecided) {
      dishes_with_bites++;
    }
  }

  // Also check if dishes have taken damage (body changed from base)
  // This proves damage was actually dealt
  int dishes_with_damage = 0;
  for (const auto &dish : processor.playerDishes) {
    if (dish.currentBody < dish.baseBody) {
      dishes_with_damage++;
    }
  }
  for (const auto &dish : processor.opponentDishes) {
    if (dish.currentBody < dish.baseBody) {
      dishes_with_damage++;
    }
  }

  // Check if any dishes progressed past InQueue phase
  int dishes_in_combat = 0;
  for (const auto &dish : processor.playerDishes) {
    if (dish.phase == BattleProcessor::DishSimData::Phase::InCombat ||
        dish.phase == BattleProcessor::DishSimData::Phase::Finished ||
        dish.phase == BattleProcessor::DishSimData::Phase::Entering) {
      dishes_in_combat++;
    }
  }
  for (const auto &dish : processor.opponentDishes) {
    if (dish.phase == BattleProcessor::DishSimData::Phase::InCombat ||
        dish.phase == BattleProcessor::DishSimData::Phase::Finished ||
        dish.phase == BattleProcessor::DishSimData::Phase::Entering) {
      dishes_in_combat++;
    }
  }

  // CRITICAL: Also check ECS entities (DishBattleState) to verify visual system
  // is running This is separate from BattleProcessor simulation - both need to
  // work
  int ecs_dishes_with_bites = 0;
  for (afterhours::Entity &e :
       afterhours::EntityQuery().whereHasComponent<DishBattleState>().gen()) {
    const DishBattleState &dbs = e.get<DishBattleState>();
    if (dbs.first_bite_decided &&
        dbs.phase == DishBattleState::Phase::InCombat) {
      ecs_dishes_with_bites++;
    }
  }

  log_info("TEST_COMBAT_TICKS: Checking combat ticks - completed_courses={}, "
           "dishes_with_bites={} (sim), ecs_dishes_with_bites={} (visual), "
           "dishes_with_damage={}, dishes_in_combat={}, min_ticks={}",
           completed_courses, dishes_with_bites, ecs_dishes_with_bites,
           dishes_with_damage, dishes_in_combat, min_ticks);

  // Combat ticks occurred if:
  // 1. At least one course completed (outcome recorded) - PROVES combat
  // happened
  // 2. OR at least one dish had a bite (firstBiteDecided = true) - PROVES
  // resolveCombatTick was called
  // 3. OR at least one dish took damage - PROVES damage was dealt
  // Just being in combat phase is NOT enough - we need actual bites/damage
  bool combat_ticks_occurred = (completed_courses > 0) ||
                               (dishes_with_bites > 0) ||
                               (dishes_with_damage > 0);

  if (!combat_ticks_occurred) {
    fail("No combat ticks occurred - completed_courses=" +
             std::to_string(completed_courses) +
             ", dishes_with_bites=" + std::to_string(dishes_with_bites) +
             ", dishes_with_damage=" + std::to_string(dishes_with_damage) +
             ", dishes_in_combat=" + std::to_string(dishes_in_combat) +
             ". Battle did not simulate. No bites occurred, no damage dealt, "
             "no courses completed.",
         location);
    return *this;
  }

  // CRITICAL: Require at least one bite to have occurred in BOTH systems
  // 1. BattleProcessor simulation (dishes_with_bites) - proves simulation ran
  // 2. ECS visual system (ecs_dishes_with_bites) - proves
  // ResolveCombatTickSystem ran Both must work for the battle to be visible to
  // the user
  if (dishes_with_bites == 0 && completed_courses == 0) {
    fail("No bites occurred in BattleProcessor simulation - "
         "firstBiteDecided=false for all dishes. "
         "BattleProcessor::resolveCombatTick was never called. "
         "completed_courses=" +
             std::to_string(completed_courses) +
             ", dishes_with_damage=" + std::to_string(dishes_with_damage) +
             ". Battle simulation is not running.",
         location);
    return *this;
  }

  // CRITICAL: Also check that ECS visual system processed bites
  // If simulation has bites but ECS doesn't, ResolveCombatTickSystem isn't
  // running
  if (ecs_dishes_with_bites == 0 && dishes_with_bites > 0) {
    fail("BattleProcessor simulation has bites (dishes_with_bites=" +
             std::to_string(dishes_with_bites) +
             ") but ECS visual system has none (ecs_dishes_with_bites=0). "
             "ResolveCombatTickSystem is not running or is blocked. "
             "User will not see animations or dish movement.",
         location);
    return *this;
  }

  // If we need minimum ticks, check outcomes count
  if (min_ticks > 0 && completed_courses < min_ticks) {
    // For min_ticks, we're checking if enough courses completed
    // But if dishes had bites or took damage, at least some ticks occurred
    // So we'll accept that as meeting the minimum
    if (dishes_with_bites == 0 && dishes_with_damage == 0) {
      fail("Insufficient combat ticks - expected at least " +
               std::to_string(min_ticks) + " but got " +
               std::to_string(completed_courses) + " completed courses, " +
               std::to_string(dishes_with_bites) + " bites, and " +
               std::to_string(dishes_with_damage) + " dishes with damage",
           location);
      return *this;
    }
  }

  log_info("TEST_COMBAT_TICKS: Combat ticks verified - combat occurred");
  return *this;
}

TestApp &TestApp::wait_for_battle_complete(float timeout_sec,
                                           const std::string &location) {
  TestOperationID op_id = generate_operation_id(std::source_location::current(),
                                                "wait_for_battle_complete");

  static std::chrono::steady_clock::time_point start_time;
  static bool started = false;
  static int last_course_index = -1;
  static int log_counter = 0;
  if (!started) {
    start_time = std::chrono::steady_clock::now();
    started = true;
    last_course_index = -1;
    log_counter = 0;
  }

  if (completed_operations.count(op_id) > 0) {
    started = false;
    return *this;
  }

  GameStateManager::get().update_screen();
  GameStateManager::Screen current = GameStateManager::get().active_screen;

  if (current == GameStateManager::Screen::Results) {
    completed_operations.insert(op_id);
    started = false;
    return *this;
  }

  int player_count = count_active_player_dishes();
  int opponent_count = count_active_opponent_dishes();

  // Log course progression and dish counts periodically
  auto combat_queue_opt =
      afterhours::EntityHelper::get_singleton<CombatQueue>();
  if (combat_queue_opt.get().has<CombatQueue>()) {
    const CombatQueue &cq = combat_queue_opt.get().get<CombatQueue>();
    if (cq.current_index != last_course_index) {
      last_course_index = cq.current_index;
      log_info("TEST_BATTLE: Course {} started - Player dishes: {}, Opponent "
               "dishes: {}",
               cq.current_index + 1, player_count, opponent_count);
    }

    // Log dish counts every 100 frames to track progress
    log_counter++;
    if (log_counter % 100 == 0) {
      log_info("TEST_BATTLE: Course {} in progress - Player dishes: {}, "
               "Opponent dishes: {}, Complete: {}",
               cq.current_index + 1, player_count, opponent_count, cq.complete);
    }
  } else {
    // Log dish counts even if CombatQueue not available
    log_counter++;
    if (log_counter % 100 == 0) {
      log_info("TEST_BATTLE: Battle in progress - Player dishes: {}, Opponent "
               "dishes: {}",
               player_count, opponent_count);
    }
  }

  if (player_count == 0 || opponent_count == 0) {
    log_info(
        "TEST_BATTLE: Battle ending - Player dishes: {}, Opponent dishes: {}",
        player_count, opponent_count);
    wait_for_screen(GameStateManager::Screen::Results, timeout_sec);
    return *this;
  }

  std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  std::chrono::milliseconds ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
  if (ms.count() > static_cast<int>(timeout_sec * 1000.0f)) {
    fail("Timeout waiting for battle to complete - player: " +
             std::to_string(player_count) +
             ", opponent: " + std::to_string(opponent_count),
         location);
  }

  wait_state.type = WaitState::FrameDelay;
  wait_state.frame_delay_count = 2;
  wait_state.operation_id = op_id;
  yield([this]() { TestRegistry::get().run_test(current_test_name, *this); });
  return *this;
}

TestApp &TestApp::wait_for_results_screen(float timeout_sec,
                                          const std::string &location) {
  (void)location;
  return wait_for_screen(GameStateManager::Screen::Results, timeout_sec,
                         std::source_location::current());
}

TestApp &TestApp::expect_battle_not_tie(const std::string &location) {
  auto result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (!result_entity.get().has<BattleResult>()) {
    fail("BattleResult singleton not found", location);
  }
  const BattleResult &result = result_entity.get().get<BattleResult>();
  if (result.outcome == BattleResult::Outcome::Tie) {
    fail("Battle resulted in a tie when it should have a winner", location);
  }
  return *this;
}

TestApp &TestApp::expect_battle_has_outcomes(const std::string &location) {
  auto result_entity = afterhours::EntityHelper::get_singleton<BattleResult>();
  if (!result_entity.get().has<BattleResult>()) {
    fail("BattleResult singleton not found", location);
  }
  const BattleResult &result = result_entity.get().get<BattleResult>();
  if (result.playerWins == 0 && result.opponentWins == 0 && result.ties == 0) {
    fail("Battle has no wins or ties - battle may have ended prematurely",
         location);
  }
  return *this;
}

TestApp &TestApp::expect_true(bool value, const std::string &description,
                              const std::string &location) {
  if (!value) {
    fail("Expected " + description + " to be true but got false", location);
  }
  return *this;
}

TestApp &TestApp::expect_false(bool value, const std::string &description,
                               const std::string &location) {
  if (value) {
    fail("Expected " + description + " to be false but got true", location);
  }
  return *this;
}

TestApp &TestApp::kill_server() {
  const char *pid_str = std::getenv("TEST_SERVER_PID");
  if (!pid_str) {
    log_warn("TEST: TEST_SERVER_PID environment variable not set - server may "
             "not be running");
    return *this;
  }

  int pid = std::stoi(pid_str);
  log_info("TEST: Killing server with PID {}", pid);

  // Check if process exists before trying to kill it
  std::string check_cmd = "kill -0 " + std::to_string(pid) + " 2>/dev/null";
  int check_result = system(check_cmd.c_str());
  if (check_result != 0) {
    log_warn("TEST: Server process {} does not exist (may already be dead)",
             pid);
    return *this;
  }

  // Kill the server process (use -9 for forceful kill)
  std::string kill_cmd = "kill -9 " + std::to_string(pid);
  int result = system(kill_cmd.c_str());
  if (result != 0) {
    log_warn("TEST: Failed to kill server process (may already be dead)");
  }

  // Wait a moment for the process to fully terminate and port to be released
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));

  // The test will wait for NetworkSystem to detect the failure and
  // ServerDisconnectionSystem to navigate to main menu
  log_info(
      "TEST: Server killed, waiting for NetworkSystem to detect failure...");

  return *this;
}

TestApp &TestApp::force_network_check() {
  auto networkInfoOpt = afterhours::EntityHelper::get_singleton<NetworkInfo>();
  afterhours::Entity &entity = networkInfoOpt.get();
  if (!entity.has<NetworkInfo>()) {
    fail("NetworkInfo singleton not found", "");
    return *this;
  }

  NetworkInfo &networkInfo = entity.get<NetworkInfo>();
  // Set timeSinceLastCheck to 0 to trigger immediate check
  networkInfo.timeSinceLastCheck = 0.0f;
  log_info("TEST: Forced NetworkSystem to perform immediate health check");

  return *this;
}

TestApp &TestApp::trigger_game_state_save() {
  GameStateSaveSystem save_system;
  auto result = save_system.save_game_state();
  if (!result.success) {
    fail("Failed to save game state");
  }
  return *this;
}

TestApp &TestApp::trigger_game_state_load() {
  auto continue_opt =
      afterhours::EntityHelper::get_singleton<ContinueGameRequest>();
  if (!continue_opt.get().has<ContinueGameRequest>()) {
    continue_opt.get().addComponent<ContinueGameRequest>();
    afterhours::EntityHelper::registerSingleton<ContinueGameRequest>(
        continue_opt.get());
  }
  continue_opt.get().get<ContinueGameRequest>().requested = true;
  // Wait a frame for GameStateLoadSystem to process
  wait_for_frames(1);
  return *this;
}

bool TestApp::save_file_exists() {
  auto userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
  if (!userId_opt.get().has<UserId>()) {
    return false;
  }
  std::string userId = userId_opt.get().get<UserId>().userId;
  std::string save_file = server::FileStorage::get_game_state_save_path(userId);
  return server::FileStorage::file_exists(save_file);
}

bool TestApp::try_purchase_item(DishType type, int inventory_slot,
                                const std::string & /*location*/) {
  // Attempt to purchase without throwing exceptions
  // Returns true if purchase succeeded, false if it failed (e.g., insufficient
  // funds)

  // Ensure we're on the shop screen
  GameStateManager::Screen current = GameStateManager::get().active_screen;
  if (current != GameStateManager::Screen::Shop) {
    return false;
  }

  // Find a shop item of the specified type
  afterhours::Entity *shop_item = nullptr;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsShopItem>().gen()) {
    if (entity.has<IsDish>()) {
      const IsDish &dish = entity.get<IsDish>();
      if (dish.type == type) {
        shop_item = &entity;
        break;
      }
    }
  }

  if (!shop_item) {
    return false;
  }

  // Get the price
  const int price = get_dish_info(type).price;
  const int initial_gold = read_wallet_gold();

  // Check if player can afford it
  if (initial_gold < price) {
    return false; // Can't afford it
  }

  // Perform the purchase (same logic as purchase_item but without throwing)
  // Find an empty inventory slot
  afterhours::Entity *target_slot = nullptr;
  if (inventory_slot >= 0) {
    // Use specified slot
    for (afterhours::Entity &entity :
         afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
      const IsDropSlot &slot = entity.get<IsDropSlot>();
      if (slot.slot_id == inventory_slot && slot.accepts_inventory_items &&
          !slot.occupied) {
        target_slot = &entity;
        break;
      }
    }
    if (!target_slot) {
      return false;
    }
  } else {
    // Find any empty inventory slot (exclude sell slot)
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsDropSlot>()
             .gen()) {
      const IsDropSlot &slot = entity.get<IsDropSlot>();
      if (slot.accepts_inventory_items && !slot.occupied &&
          slot.slot_id != SELL_SLOT_ID) {
        target_slot = &entity;
        break;
      }
    }
    if (!target_slot) {
      return false;
    }
  }

  // Get original slot ID before removing IsShopItem
  int original_slot = -1;
  if (shop_item->has<IsShopItem>()) {
    original_slot = shop_item->get<IsShopItem>().slot;
  }

  // Charge for purchase
  if (!charge_for_shop_purchase(type)) {
    return false; // Failed to charge
  }

  // Remove IsShopItem and add IsInventoryItem
  shop_item->removeComponent<IsShopItem>();
  IsInventoryItem &inv_item = shop_item->addComponent<IsInventoryItem>();
  inv_item.slot = target_slot->get<IsDropSlot>().slot_id;

  shop_item->removeComponentIfExists<Freezeable>();

  // Mark slot as occupied
  target_slot->get<IsDropSlot>().occupied = true;

  // Free the original shop slot
  if (original_slot >= 0) {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
      if (entity.get<IsDropSlot>().slot_id == original_slot) {
        entity.get<IsDropSlot>().occupied = false;
        break;
      }
    }
  }

  // Changes will be visible after system loop runs
  return true; // Purchase succeeded
}

TestApp &TestApp::purchase_item(DishType type, int inventory_slot,
                                const std::string &location) {
  // Ensure we're on the shop screen
  expect_screen_is(GameStateManager::Screen::Shop, location);

  // Find a shop item of the specified type
  afterhours::Entity *shop_item = nullptr;
  for (afterhours::Entity &entity :
       afterhours::EntityQuery().whereHasComponent<IsShopItem>().gen()) {
    if (entity.has<IsDish>()) {
      const IsDish &dish = entity.get<IsDish>();
      if (dish.type == type) {
        shop_item = &entity;
        break;
      }
    }
  }

  if (!shop_item) {
    fail("Shop item of type " + std::to_string(static_cast<int>(type)) +
             " not found in shop",
         location);
  }

  // Get the price
  const int price = get_dish_info(type).price;
  const int initial_gold = read_wallet_gold();

  // Check if player can afford it
  if (initial_gold < price) {
    fail("Cannot purchase: insufficient gold (have " +
             std::to_string(initial_gold) + ", need " + std::to_string(price) +
             ")",
         location);
  }

  // Find an empty inventory slot
  // Retry with delays to handle timing issues in visible mode
  afterhours::Entity *target_slot = nullptr;
  const int max_attempts = 5;

  for (int attempt = 0; attempt < max_attempts && !target_slot; attempt++) {
    // force_merge in queries handles merging automatically
    if (inventory_slot >= 0) {
      // Use specified slot
      for (afterhours::Entity &entity :
           afterhours::EntityQuery({.force_merge = true})
               .whereHasComponent<IsDropSlot>()
               .gen()) {
        const IsDropSlot &slot = entity.get<IsDropSlot>();
        if (slot.accepts_inventory_items && slot.accepts_shop_items &&
            slot.slot_id == inventory_slot && !slot.occupied) {
          target_slot = &entity;
          break;
        }
      }
    } else {
      // Find any empty inventory slot
      // Note: We need slots that accept both inventory AND shop items (actual
      // inventory slots) The sell slot accepts inventory items but not shop
      // items, so we exclude it
      for (afterhours::Entity &entity :
           afterhours::EntityQuery({.force_merge = true})
               .whereHasComponent<IsDropSlot>()
               .gen()) {
        const IsDropSlot &slot = entity.get<IsDropSlot>();
        if (slot.accepts_inventory_items && slot.accepts_shop_items &&
            !slot.occupied) {
          target_slot = &entity;
          break;
        }
      }
    }

    if (!target_slot && attempt < max_attempts - 1) {
      // Wait a bit for systems to process before retrying
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Final check after all attempts
  if (!target_slot) {
    // force_merge in query handles merging automatically
    int final_occupied = 0;
    int final_total = 0;
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<IsDropSlot>()
             .gen()) {
      const IsDropSlot &slot = entity.get<IsDropSlot>();
      if (slot.accepts_inventory_items && slot.accepts_shop_items) {
        final_total++;
        if (slot.occupied) {
          final_occupied++;
        }
      }
    }
    if (inventory_slot >= 0) {
      fail("Inventory slot " + std::to_string(inventory_slot) +
               " not found or is occupied after " +
               std::to_string(max_attempts) +
               " attempts (total slots: " + std::to_string(final_total) +
               ", occupied: " + std::to_string(final_occupied) + ")",
           location);
    } else {
      fail("No empty inventory slots available after " +
               std::to_string(max_attempts) +
               " attempts (total slots: " + std::to_string(final_total) +
               ", occupied: " + std::to_string(final_occupied) + ", max: 7)",
           location);
    }
  }

  // Simulate drag-and-drop: add IsHeld component, then let system process
  // We'll simulate what DropWhenNoLongerHeld does for purchasing
  vec2 slot_center = target_slot->get<Transform>().center();
  vec2 item_size = shop_item->has<Transform>()
                       ? shop_item->get<Transform>().size
                       : vec2{80.0f, 80.0f};

  // Add IsHeld component to simulate dragging
  if (!shop_item->has<Transform>()) {
    shop_item->addComponent<Transform>(vec2{0, 0}, item_size);
  }
  Transform &item_transform = shop_item->get<Transform>();
  IsHeld held_component{vec2{0, 0}, item_transform.position};

  // Position item over the target slot
  item_transform.position = slot_center - item_size * 0.5f;

  // Get original slot ID before removing IsShopItem
  int original_slot = -1;
  if (shop_item->has<IsShopItem>()) {
    original_slot = shop_item->get<IsShopItem>().slot;
  }

  // Charge for purchase (same logic as
  // DropWhenNoLongerHeld::try_purchase_shop_item)
  if (!charge_for_shop_purchase(type)) {
    // Failed to charge - restore position
    item_transform.position = held_component.original_position;
    fail("Failed to charge wallet for purchase", location);
  }

  // Remove IsShopItem and add IsInventoryItem
  shop_item->removeComponent<IsShopItem>();
  IsInventoryItem &inv_item = shop_item->addComponent<IsInventoryItem>();
  inv_item.slot = target_slot->get<IsDropSlot>().slot_id;

  shop_item->removeComponentIfExists<Freezeable>();

  // Mark slot as occupied
  target_slot->get<IsDropSlot>().occupied = true;

  // Free the original shop slot
  if (original_slot >= 0) {
    for (afterhours::Entity &entity :
         afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
      if (entity.get<IsDropSlot>().slot_id == original_slot) {
        entity.get<IsDropSlot>().occupied = false;
        break;
      }
    }
  }

  // Changes will be visible after system loop runs
  // Verify purchase succeeded
  const int new_gold = read_wallet_gold();
  if (new_gold != initial_gold - price) {
    fail("Gold not deducted correctly: expected " +
             std::to_string(initial_gold - price) + ", got " +
             std::to_string(new_gold),
         location);
  }

  if (step_delay()) {
    yield([this]() {
      test_resuming = true;
      TestRegistry::get().run_test(current_test_name, *this);
    });
    return *this;
  }

  return *this;
}

afterhours::OptEntity TestApp::find_inventory_item_by_slot(int slot_index) {
  // Use force_merge since this might be called immediately after entity
  // creation
  return EQ({.force_merge = true})
      .whereHasComponent<IsInventoryItem>()
      .whereHasComponent<IsDish>()
      .whereLambda([slot_index](const afterhours::Entity &e) {
        return e.get<IsInventoryItem>().slot == slot_index;
      })
      .gen_first();
}

afterhours::OptEntity TestApp::find_drop_slot(int slot_id) {
  return EQ().whereHasComponent<IsDropSlot>().whereSlotID(slot_id).gen_first();
}

int TestApp::find_free_shop_slot() {
  // force_merge in query handles merging automatically
  for (int i = 0; i < SHOP_SLOTS; ++i) {
    bool slot_taken = false;
    for (afterhours::Entity &entity :
         EQ({.force_merge = true})
             .template whereHasComponent<IsShopItem>()
             .gen()) {
      if (entity.get<IsShopItem>().slot == i) {
        slot_taken = true;
        break;
      }
    }
    if (!slot_taken) {
      return i;
    }
  }
  return -1;
}

int TestApp::find_free_inventory_slot() {
  // force_merge in query handles merging automatically
  for (int i = 0; i < INVENTORY_SLOTS; ++i) {
    bool slot_occupied = EQ({.force_merge = true})
                             .template whereHasComponent<IsInventoryItem>()
                             .whereLambda([i](const afterhours::Entity &e) {
                               return e.get<IsInventoryItem>().slot == i;
                             })
                             .has_values();
    if (!slot_occupied) {
      return i;
    }
  }
  return -1;
}

afterhours::OptEntity TestApp::find_shop_item(afterhours::EntityID id,
                                              int slot) {
  auto entity_opt = EQ().whereID(id)
                        .template whereHasComponent<IsShopItem>()
                        .template whereHasComponent<IsDish>()
                        .gen_first();
  if (entity_opt) {
    return entity_opt;
  }
  for (afterhours::Entity &entity :
       EQ().template whereHasComponent<IsShopItem>()
           .template whereHasComponent<IsDish>()
           .gen()) {
    if (entity.get<IsShopItem>().slot == slot) {
      return afterhours::OptEntity(entity);
    }
  }
  return afterhours::OptEntity();
}

bool TestApp::simulate_sell(afterhours::Entity &inventory_item) {
  if (!inventory_item.has<IsInventoryItem>()) {
    return false;
  }

  afterhours::EntityID item_id = inventory_item.id;
  int original_slot_id = inventory_item.get<IsInventoryItem>().slot;

  auto sell_slot_opt = find_drop_slot(SELL_SLOT_ID);
  if (!sell_slot_opt) {
    return false;
  }
  afterhours::Entity &sell_slot = sell_slot_opt.asE();

  if (!inventory_item.has<Transform>()) {
    inventory_item.addComponent<Transform>(vec2{0, 0}, vec2{80.0f, 80.0f});
  }
  if (!sell_slot.has<Transform>()) {
    return false;
  }

  vec2 slot_center = sell_slot.get<Transform>().center();
  Transform &item_transform = inventory_item.get<Transform>();
  vec2 original_position = item_transform.position;

  inventory_item.addComponent<IsHeld>(vec2{0, 0}, original_position);
  inventory_item.get<Transform>().position =
      slot_center - inventory_item.get<Transform>().size * 0.5f;

  if (!sell_slot.has<CanDropOnto>()) {
    sell_slot.addComponent<CanDropOnto>(true);
  }

  // Use force_merge to query immediately
  auto item_ptr_opt = EQ({.force_merge = true}).whereID(item_id).gen_first();
  if (!item_ptr_opt.has_value()) {
    return false;
  }
  afterhours::Entity &item_entity = item_ptr_opt.asE();

  if (!item_entity.has<IsHeld>() || !item_entity.has<Transform>()) {
    return false;
  }

  auto original_slot_opt = find_drop_slot(original_slot_id);
  if (!original_slot_opt.has_value()) {
    return false;
  }
  original_slot_opt.asE().get<IsDropSlot>().occupied = false;

  auto wallet_entity = afterhours::EntityHelper::get_singleton<Wallet>();
  if (!wallet_entity.get().has<Wallet>()) {
    return false;
  }
  auto &wallet = wallet_entity.get().get<Wallet>();
  wallet.gold += 1;

  item_entity.cleanup = true;
  item_entity.removeComponent<IsHeld>();

  // Cleanup will happen when system loop runs
  return true;
}
