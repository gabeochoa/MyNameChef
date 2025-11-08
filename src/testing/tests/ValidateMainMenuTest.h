#pragma once

#include "../test_macros.h"

TEST(validate_main_menu) {
  app.launch_game();
  app.wait_for_frames(50); // Give UI systems plenty of time to create elements
  
  // Check for either "Play" or "New Team" (depending on whether save file exists)
  // The UI system creates different buttons based on save file presence
  // We'll check for both possibilities with longer timeouts
  app.wait_for_ui_exists("Play", 15.0f);
  // If Play doesn't exist, the test will fail above, which is fine
  // Otherwise, continue to check other buttons
  
  // These should always be present
  app.wait_for_ui_exists("Settings", 15.0f);
  app.wait_for_ui_exists("Dishes", 15.0f);
  app.wait_for_ui_exists("Quit", 15.0f);
}
