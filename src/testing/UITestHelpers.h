#pragma once

#include <cassert>
#include <functional>
#include <optional>
#include <string>

#include <afterhours/ah.h>
#include <afterhours/src/plugins/autolayout.h>
#include <afterhours/src/plugins/ui/components.h>

// Include the component headers we need for entity validation
#include "../components/is_drop_slot.h"
#include "../components/is_inventory_item.h"
#include "../components/is_shop_item.h"
#include "../game_state_manager.h" // For screen transition checking
#include "../log.h"

struct UITestHelpers {
  // Sleep/wait functionality for test validation
  static bool sleep_for_transition(int frames_to_wait = 10) {
    static int frames_waited = 0;
    frames_waited++;

    if (frames_waited >= frames_to_wait) {
      frames_waited = 0; // Reset for next use
      return true;       // Sleep complete
    }
    return false; // Still sleeping
  }

  // Wait for screen transition by checking actual screen value
  static bool
  wait_for_screen_transition(GameStateManager::Screen expected_screen,
                             int max_frames = 30) {
    static int frames_waited = 0;
    frames_waited++;

    auto &gsm = GameStateManager::get();
    if (gsm.active_screen == expected_screen) {
      log_info("DEBUG: Screen transition complete to {} after {} frames",
               static_cast<int>(expected_screen), frames_waited);
      frames_waited = 0; // Reset for next use
      return true;       // Transition complete
    }

    if (frames_waited >= max_frames) {
      log_info("DEBUG: Screen transition timeout after {} frames, still on "
               "screen {}",
               frames_waited, static_cast<int>(gsm.active_screen));
      frames_waited = 0; // Reset for next use
      return false;      // Timeout
    }

    return false; // Still waiting
  }

  // Debug function to list all UI elements with labels
  static void debug_list_all_ui_elements() {
    log_info("DEBUG: Listing all UI elements with labels:");
    int count = 0;
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<afterhours::ui::HasLabel>()
                         .gen()) {
      auto &entity = ref.get();
      if (entity.has<afterhours::ui::HasLabel>()) {
        auto &ui_label = entity.get<afterhours::ui::HasLabel>();
        log_info("DEBUG: Found UI element #{}: '{}'", count++, ui_label.label);
      }
    }
    log_info("DEBUG: Total UI elements found: {}", count);
  }

  // Check if a UI element exists (returns bool, for validation functions)
  static bool check_ui_exists(const std::string &label, int max_attempts = 3) {
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
      for (auto &ref : afterhours::EntityQuery()
                           .whereHasComponent<afterhours::ui::HasLabel>()
                           .gen()) {
        auto &entity = ref.get();
        if (entity.has<afterhours::ui::HasLabel>()) {
          auto &ui_label = entity.get<afterhours::ui::HasLabel>();
          if (ui_label.label == label) {
            return true; // Found the label
          }
        }
      }

      // If not found and not the last attempt, wait before retrying
      if (attempt < max_attempts) {
        // Simple delay - just do some work to give time for UI to load
        volatile int dummy = 0;
        for (int i = 0; i < 10000; i++) {
          dummy += i;
        }
      }
    }
    return false;
  }

  // Assert that a UI element with the given label is visible (with retry logic)
  static void assert_ui_exists(const std::string &label, int max_attempts = 3) {
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
      for (auto &ref : afterhours::EntityQuery()
                           .whereHasComponent<afterhours::ui::HasLabel>()
                           .gen()) {
        auto &entity = ref.get();
        if (entity.has<afterhours::ui::HasLabel>()) {
          auto &ui_label = entity.get<afterhours::ui::HasLabel>();
          if (ui_label.label == label) {
            log_info("TEST ASSERT: UI element '{}' found on attempt {}", label,
                     attempt);
            return; // Found the label, assertion passes
          }
        }
      }

      // If not found and not the last attempt, wait before retrying
      if (attempt < max_attempts) {
        log_info(
            "TEST ASSERT: UI element '{}' not found on attempt {}, retrying...",
            label, attempt);
        // Simple delay - just do some work to give time for UI to load
        volatile int dummy = 0;
        for (int i = 0; i < 10000; i++) {
          dummy += i;
        }
      }
    }

    // If we get here, the UI element was not found after all attempts
    log_error("TEST ASSERT FAILED: UI element '{}' not found after {} attempts",
              label, max_attempts);
    assert(false && "UI element not found");
  }

  // Find a UI element by label
  static std::optional<afterhours::Entity *>
  find_ui_element(const std::string &label) {
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<afterhours::ui::HasLabel>()
                         .gen()) {
      auto &entity = ref.get();
      if (entity.has<afterhours::ui::HasLabel>()) {
        auto &ui_label = entity.get<afterhours::ui::HasLabel>();
        if (ui_label.label == label) {
          return &entity;
        }
      }
    }
    return std::nullopt;
  }

  // Assert and click a UI element by label (with retry logic)
  static void assert_click_ui(const std::string &label, int max_attempts = 3) {
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
      auto element = find_ui_element(label);
      if (element.has_value()) {
        auto &entity = *element.value();
        log_info("DEBUG: Found UI element '{}', checking for click listener",
                 label);
        if (entity.has<afterhours::ui::HasClickListener>()) {
          auto &click_listener = entity.get<afterhours::ui::HasClickListener>();
          log_info("DEBUG: Calling click callback for '{}'", label);
          click_listener.cb(entity);
          log_info("DEBUG: Click callback completed for '{}'", label);
          log_info(
              "TEST ASSERT: Successfully clicked UI element '{}' on attempt {}",
              label, attempt);
          return; // Click successful, assertion passes
        } else {
          log_info("DEBUG: UI element '{}' found but has no HasClickListener",
                   label);
        }
      } else {
        log_info("DEBUG: UI element '{}' not found on attempt {}", label,
                 attempt);
      }

      // If not found and not the last attempt, wait before retrying
      if (attempt < max_attempts) {
        log_info("TEST ASSERT: UI element '{}' not clickable on attempt {}, "
                 "retrying...",
                 label, attempt);
        // Simple delay - just do some work to give time for UI to load
        volatile int dummy = 0;
        for (int i = 0; i < 10000; i++) {
          dummy += i;
        }
      }
    }

    // If we get here, the UI element was not clickable after all attempts
    log_error(
        "TEST ASSERT FAILED: UI element '{}' not clickable after {} attempts",
        label, max_attempts);
    assert(false && "UI element not clickable");
  }

  // Check if a UI element contains specific text
  static bool ui_contains_text(const std::string &label,
                               const std::string &expected_text) {
    auto element = find_ui_element(label);
    if (!element.has_value()) {
      return false;
    }

    auto &entity = *element.value();
    if (entity.has<afterhours::ui::HasLabel>()) {
      auto &ui_label = entity.get<afterhours::ui::HasLabel>();
      return ui_label.label.find(expected_text) != std::string::npos;
    }
    return false;
  }

  // Get the text content of a UI element
  static std::optional<std::string> get_ui_text(const std::string &label) {
    auto element = find_ui_element(label);
    if (!element.has_value()) {
      return std::nullopt;
    }

    auto &entity = *element.value();
    if (entity.has<afterhours::ui::HasLabel>()) {
      auto &ui_label = entity.get<afterhours::ui::HasLabel>();
      return ui_label.label;
    }
    return std::nullopt;
  }

  // Count how many UI elements with a given label exist
  static int count_ui_elements(const std::string &label) {
    int count = 0;
    for (auto &ref : afterhours::EntityQuery()
                         .whereHasComponent<afterhours::ui::HasLabel>()
                         .gen()) {
      auto &entity = ref.get();
      if (entity.has<afterhours::ui::HasLabel>()) {
        auto &ui_label = entity.get<afterhours::ui::HasLabel>();
        if (ui_label.label == label) {
          count++;
        }
      }
    }
    return count;
  }

  // ===== ENTITY VALIDATION FUNCTIONS =====
  // These functions validate visual elements (entities with components) rather
  // than UI elements

  // Check if entities with specific components exist (for visual elements)
  static bool entities_exist_with_component(const std::string &component_name) {
    if (component_name == "IsDropSlot") {
      return afterhours::EntityQuery()
          .whereHasComponent<IsDropSlot>()
          .has_values();
    }
    // Add more component checks as needed
    return false;
  }

  // Count entities with specific components
  static int count_entities_with_component(const std::string &component_name) {
    if (component_name == "IsDropSlot") {
      return static_cast<int>(afterhours::EntityQuery()
                                  .whereHasComponent<IsDropSlot>()
                                  .gen_count());
    }
    // Add more component checks as needed
    return 0;
  }

  // Check if shop slots exist (visual elements, not UI elements) with retry
  // logic
  static bool shop_slots_exist(int max_attempts = 3) {
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
      int shop_slot_count = 0;
      for (auto &ref :
           afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
        auto &entity = ref.get();
        if (entity.has<IsDropSlot>()) {
          auto &drop_slot = entity.get<IsDropSlot>();
          // Shop slots accept shop items but not inventory items
          if (!drop_slot.accepts_inventory_items &&
              drop_slot.accepts_shop_items) {
            shop_slot_count++;
          }
        }
      }

      if (shop_slot_count >= 7) { // Expected 7 shop slots
        return true;
      }

      // If not found and not the last attempt, wait before retrying
      if (attempt < max_attempts) {
        // Simple delay - just do some work to give time for entities to load
        volatile int dummy = 0;
        for (int i = 0; i < 10000; i++) {
          dummy += i;
        }
      }
    }
    return false;
  }

  // Check if inventory slots exist (visual elements, not UI elements)
  static bool inventory_slots_exist() {
    int inventory_slot_count = 0;
    for (auto &ref :
         afterhours::EntityQuery().whereHasComponent<IsDropSlot>().gen()) {
      auto &entity = ref.get();
      if (entity.has<IsDropSlot>()) {
        auto &drop_slot = entity.get<IsDropSlot>();
        // Inventory slots accept both inventory and shop items
        if (drop_slot.accepts_inventory_items && drop_slot.accepts_shop_items) {
          inventory_slot_count++;
        }
      }
    }
    return inventory_slot_count >= 7; // Expected 7 inventory slots
  }

  // Check if shop items exist (dishes in shop slots)
  static bool shop_items_exist() {
    return afterhours::EntityQuery()
               .whereHasComponent<IsShopItem>()
               .gen_count() > 0;
  }

  // Check if inventory items exist (dishes in inventory slots)
  static bool inventory_items_exist() {
    return afterhours::EntityQuery()
               .whereHasComponent<IsInventoryItem>()
               .gen_count() > 0;
  }
};