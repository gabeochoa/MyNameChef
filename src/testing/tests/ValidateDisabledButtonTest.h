#pragma once

#include "../../game_state_manager.h"
#include "../test_macros.h"
#include <afterhours/ah.h>

TEST(validate_disabled_button) {
  app.launch_game();
  app.wait_for_ui_exists("Play");

  // Find the Play button entity
  afterhours::Entity *play_button = nullptr;
  for (auto &ref : afterhours::EntityQuery()
                       .whereHasComponent<afterhours::ui::HasLabel>()
                       .whereHasComponent<afterhours::ui::HasClickListener>()
                       .gen()) {
    auto &entity = ref.get();
    if (entity.has<afterhours::ui::HasLabel>()) {
      auto &label = entity.get<afterhours::ui::HasLabel>();
      if (label.label == "Play") {
        play_button = &entity;
        break;
      }
    }
  }

  app.expect_not_empty(play_button != nullptr, "Play button should exist");

  // Check if button is disabled (when network is disconnected, Play button is
  // disabled)
  bool is_disabled = false;
  if (play_button->has<afterhours::ui::HasLabel>()) {
    is_disabled = play_button->get<afterhours::ui::HasLabel>().is_disabled;
  }

  // If button is disabled, verify clicking it doesn't trigger navigation
  if (is_disabled) {
    GameStateManager::Screen initial_screen =
        GameStateManager::get().active_screen;

    // Attempt to click the disabled button
    // Note: The test framework's click() method directly calls the callback,
    // but in real gameplay, HandleClicks system should prevent this.
    // This test documents the current bug: disabled buttons are still
    // clickable.
    app.click("Play");
    app.wait_for_frames(5);

    // Verify screen didn't change (callback should not have been called)
    // BUG: Currently this will fail because disabled buttons are still
    // clickable
    GameStateManager::Screen after_click_screen =
        GameStateManager::get().active_screen;
    app.expect_true(initial_screen == after_click_screen,
                    "Disabled button should not trigger callback - screen "
                    "should not change");
  } else {
    // If button is not disabled, verify it works normally
    GameStateManager::Screen initial_screen =
        GameStateManager::get().active_screen;
    app.click("Play");
    app.wait_for_screen(GameStateManager::Screen::Shop, 10.0f);
    GameStateManager::Screen after_click_screen =
        GameStateManager::get().active_screen;
    app.expect_true(
        initial_screen != after_click_screen,
        "Enabled button should trigger callback - screen should change");
  }
}
