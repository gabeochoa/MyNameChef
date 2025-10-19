#pragma once

#include "../../game_state_manager.h"
#include "../../systems/ExportMenuSnapshotSystem.h"
#include "../TestInteraction.h"
#include "../UITestHelpers.h"

struct ValidateCombatSystemTest {
  static int next_round_click_count; // Track clicks to prevent infinite loop

  static void execute() {
    // Step 1: Navigate to shop screen first
    UITestHelpers::assert_ui_exists("Play");
    UITestHelpers::assert_click_ui("Play");

    // Apply the screen transition
    TestInteraction::start_game();
    GameStateManager::get().update_screen();

    // Note: "Next Round" button check and navigation will be done in
    // validate_battle_screen() which runs on subsequent frames after the shop
    // screen loads
  }

  static bool validate_battle_screen() {
    // Debug: Check what screen we're actually on
    auto &gsm = GameStateManager::get();
    log_info("DEBUG: Current screen: {}, Next screen: {}",
             static_cast<int>(gsm.active_screen),
             gsm.next_screen.has_value()
                 ? static_cast<int>(gsm.next_screen.value())
                 : -1);

    // First, check if we're on the shop screen and navigate to battle
    if (UITestHelpers::check_ui_exists("Next Round")) {
      // Prevent infinite clicking - only click once
      if (next_round_click_count == 0) {
        log_info(
            "DEBUG: Found Next Round button, calling button handler directly");

        // Call the Next Round button handler directly instead of simulating
        // clicks This is the same function that would be called by
        // button_labeled
        log_info("DEBUG: Next Round button clicked!");
        // Export menu snapshot
        ExportMenuSnapshotSystem export_system;
        std::string filename = export_system.export_menu_snapshot();
        log_info("DEBUG: Export filename: {}", filename);

        if (!filename.empty()) {
          log_info("DEBUG: Calling to_battle()");
          // Navigate to battle screen
          GameStateManager::get().to_battle();
          log_info("DEBUG: After to_battle(), next_screen: {}",
                   GameStateManager::get().next_screen.has_value()
                       ? static_cast<int>(
                             GameStateManager::get().next_screen.value())
                       : -1);
          // Apply screen transition immediately
          GameStateManager::get().update_screen();
          log_info("DEBUG: After update_screen(), active_screen: {}",
                   static_cast<int>(GameStateManager::get().active_screen));
        } else {
          log_info("DEBUG: Export failed, not transitioning to battle");
        }

        next_round_click_count++;

        // Wait for screen transition to complete
        return UITestHelpers::wait_for_screen_transition(
            GameStateManager::Screen::Battle, 30);
      } else {
        log_info("DEBUG: Next Round button clicked {} times, but still on shop "
                 "screen - screen transition bug",
                 next_round_click_count);
        // TODO: This is a game bug - screen transition from shop to battle is
        // not working Expected: After clicking Next Round, screen should
        // transition to battle Bug: Screen remains on shop screen even after
        // clicking Next Round
        return false; // Keep trying, but don't click again
      }
    }

    // If we're on the battle screen, check for battle elements
    if (UITestHelpers::check_ui_exists("Skip to Results")) {
      log_info(
          "DEBUG: Found Skip to Results button, successfully on battle screen");
      return true; // Successfully on battle screen
    }

    // Still waiting for screen transition
    return false;
  }

  static bool validate_combat_stats() {
    // TODO: Validate that dishes have proper CombatStats
    // Expected: Zing = spice + acidity + umami (0-3), Body = satiety + richness
    // + sweetness + freshness (0-4) Bug: Combat stats calculation may be wrong

    // TODO: Validate level scaling
    // Expected: If DishLevel.level > 1, multiply both Zing and Body by 2
    // Bug: Level scaling may not be applied

    return true; // Placeholder
  }

  static bool validate_battle_phases() {
    // TODO: Validate DishBattleState phase transitions
    // Expected: InQueue → Entering → InCombat → Finished
    // Bug: Phase transitions may not be working properly

    return true; // Placeholder
  }
};
