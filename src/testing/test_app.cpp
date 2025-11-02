#include "test_app.h"
#include "../components/battle_team_tags.h"
#include "../components/combat_stats.h"
#include "../components/dish_battle_state.h"
#include "../components/dish_level.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../log.h"
#include "../render_backend.h"
#include "../shop.h"
#include <afterhours/ah.h>
#include <chrono>
#include <raylib/raylib.h>
#include <thread>

void TestApp::fail(const std::string &message, const std::string &location) {
  failure_message = message;
  failure_location = location;
  log_error("TEST FAILED [{}]: {} at {}", current_test_name, message,
            location.empty() ? "unknown location" : location);
  throw std::runtime_error(message);
}

TestApp &TestApp::launch_game() {
  // Only launch once per test to avoid resetting state on test re-runs
  if (game_launched) {
    return *this;
  }
  // Reset to main menu for test isolation
  GameStateManager::get().set_next_screen(GameStateManager::Screen::Main);
  GameStateManager::get().update_screen();
  game_launched = true;
  if (step_delay()) {
    throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
  }
  return *this;
}

TestApp &TestApp::click(const std::string &button_label) {
  afterhours::Entity *entity = find_clickable_with(button_label);
  if (!entity) {
    fail("Button not found: " + button_label);
  }

  // Check if clicking this button would transition to a screen we're already on
  // or transitioning to. We need to check this after finding the entity to
  // ensure the button exists, but before clicking to avoid duplicate clicks.
  GameStateManager &gsm = GameStateManager::get();
  GameStateManager::Screen current_screen = gsm.active_screen;
  std::optional<GameStateManager::Screen> next_screen = gsm.next_screen;

  // If we're already on Shop screen, don't click "Play" again
  if (button_label == "Play" &&
      current_screen == GameStateManager::Screen::Shop) {
    return *this;
  }
  // If we're transitioning to Shop, don't click "Play" again
  if (button_label == "Play" && next_screen.has_value() &&
      next_screen.value() == GameStateManager::Screen::Shop) {
    return *this;
  }

  click_clickable(*entity);

  // Don't call update_screen() here - let the normal game loop handle it
  // The screen transition will happen on the next frame naturally
  if (step_delay()) {
    throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
  }
  return *this;
}

afterhours::Entity *TestApp::find_clickable_with(const std::string &label) {
  for (const std::shared_ptr<afterhours::Entity> &ep :
       afterhours::EntityHelper::get_entities()) {
    afterhours::Entity &e = *ep;
    if (e.has<afterhours::ui::UIComponent>() &&
        e.has<afterhours::ui::HasClickListener>()) {
      std::string name;
      if (e.has<afterhours::ui::HasLabel>()) {
        name = e.get<afterhours::ui::HasLabel>().label;
      }
      if (name.empty() && e.has<afterhours::ui::UIComponentDebug>()) {
        name = e.get<afterhours::ui::UIComponentDebug>().name();
      }
      if (name == label) {
        return &e;
      }
    }
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

TestApp &TestApp::navigate_to_shop() {
  wait_for_ui_exists("Play");
  click("Play");
  wait_for_screen(GameStateManager::Screen::Shop);
  if (step_delay()) {
    throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
  }
  return *this;
}

TestApp &TestApp::navigate_to_battle() {
  GameStateManager &gsm = GameStateManager::get();
  if (gsm.active_screen != GameStateManager::Screen::Shop) {
    fail("Must be on Shop screen to navigate to battle");
  }
  click("Next Round");
  wait_for_screen(GameStateManager::Screen::Battle);
  if (step_delay()) {
    throw std::runtime_error("WAIT_FOR_FRAME_DELAY_CONTINUE");
  }
  return *this;
}

std::vector<TestDishInfo> TestApp::read_player_inventory() {
  std::vector<TestDishInfo> result;
  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsInventoryItem>()
                                        .whereHasComponent<IsDish>()
                                        .gen()) {
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

std::vector<TestShopItemInfo> TestApp::read_store_options() {
  std::vector<TestShopItemInfo> result;
  for (afterhours::Entity &entity : afterhours::EntityQuery()
                                        .whereHasComponent<IsShopItem>()
                                        .whereHasComponent<IsDish>()
                                        .gen()) {
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
                                     const std::string &location) {
  // If we're already waiting for this UI element, don't restart the wait
  if (wait_state.type == WaitState::UI && wait_state.target_ui_label == label) {
    // Already waiting for this element, just check if it's ready
    if (check_wait_conditions()) {
      wait_state.type = WaitState::None;
      // Don't delay after wait completes - frames already processed during wait
      return *this;
    }
    // Still waiting, throw continue exception
    throw std::runtime_error("WAIT_FOR_UI_CONTINUE");
  }

  setup_wait_state(WaitState::UI, timeout_sec, location);
  wait_state.target_ui_label = label;

  // Check immediately - if found, clear wait state
  if (check_wait_conditions()) {
    wait_state.type = WaitState::None;
    // Don't delay after wait completes - frames already processed during wait
    return *this;
  }

  // Not ready yet - will be checked on next frame
  // Throw special exception to indicate we need to continue
  throw std::runtime_error("WAIT_FOR_UI_CONTINUE");
}

bool TestApp::check_wait_conditions() {
  if (wait_state.type == WaitState::None) {
    return true; // No wait condition
  }

  if (wait_state.type == WaitState::FrameDelay) {
    wait_state.frame_delay_count--;
    if (wait_state.frame_delay_count <= 0) {
      wait_state.type = WaitState::None;
      return true; // Frame delay complete
    }
    return false; // Still waiting for frames
  }

  if (wait_state.type == WaitState::UI) {
    // Merge entity arrays to ensure UI entities are available
    afterhours::EntityHelper::merge_entity_arrays();

    // Check if UI element exists (use EntityQuery like UITestHelpers does)
    for (afterhours::Entity &entity :
         afterhours::EntityQuery({.force_merge = true})
             .whereHasComponent<afterhours::ui::HasLabel>()
             .gen()) {
      const afterhours::ui::HasLabel &label =
          entity.get<afterhours::ui::HasLabel>();
      if (label.label == wait_state.target_ui_label) {
        wait_state.type = WaitState::None;
        return true; // Found it!
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
        wait_state.type = WaitState::None;
        return true; // Found it!
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
    GameStateManager::get().update_screen();
    GameStateManager::Screen current_screen =
        GameStateManager::get().active_screen;
    if (current_screen == wait_state.target_screen) {
      // Screen has transitioned, but we need to ensure the UI system has
      // had a chance to create the UI elements for this screen. Merge entity
      // arrays to ensure we can see the new UI elements.
      afterhours::EntityHelper::merge_entity_arrays();

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
          if (label.label == "Next Round" || label.label == "Reroll (5)") {
            shop_ui_exists = true;
            break;
          }
        }
        if (!shop_ui_exists) {
          return false; // Still waiting for UI to load
        }
      }

      wait_state.type = WaitState::None;
      return true; // Screen transition complete
    }

    // Check timeout
    std::chrono::steady_clock::time_point now =
        std::chrono::steady_clock::now();
    std::chrono::milliseconds ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - wait_state.start_time);
    if (ms.count() > static_cast<int>(wait_state.timeout_sec * 1000.0f)) {
      fail("Timeout waiting for screen: " +
               std::to_string(static_cast<int>(wait_state.target_screen)),
           wait_state.location);
      return true; // Timeout - fail
    }

    return false; // Still waiting
  }

  return true;
}

TestApp &TestApp::expect_screen_is(GameStateManager::Screen screen,
                                   const std::string &location) {
  GameStateManager::Screen current = GameStateManager::get().active_screen;
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
                                  float timeout_sec) {
  // If we're already waiting for this screen, don't restart the wait
  if (wait_state.type == WaitState::Screen &&
      wait_state.target_screen == screen) {
    // Already waiting for this screen, just check if it's ready
    GameStateManager::get().update_screen();
    if (check_wait_conditions()) {
      wait_state.type = WaitState::None;
      // Don't delay after wait completes - frames already processed during wait
      return *this;
    }
    // Still waiting, throw continue exception
    throw std::runtime_error("WAIT_FOR_SCREEN_CONTINUE");
  }

  setup_wait_state(WaitState::Screen, timeout_sec);
  wait_state.target_screen = screen;

  // Check immediately - if on target screen, clear wait state
  GameStateManager::get().update_screen();
  if (check_wait_conditions()) {
    wait_state.type = WaitState::None;
    // Don't delay after wait completes - frames already processed during wait
    return *this;
  }

  // Not ready yet - will be checked on next frame
  // Throw special exception to indicate we need to continue
  throw std::runtime_error("WAIT_FOR_SCREEN_CONTINUE");
}

TestApp &TestApp::wait_for_frames(int frames) {
  for (int i = 0; i < frames; ++i) {
    pump_frame();
  }
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

  afterhours::EntityHelper::merge_entity_arrays();

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

    // Return true to indicate caller should throw exception
    return true;
  }
  return false; // No delay needed
}
