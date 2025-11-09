#pragma once

#include "../../components/user_id.h"
#include "../../server/file_storage.h"
#include "../test_macros.h"
#include <afterhours/ah.h>
#include <filesystem>

TEST(validate_main_menu) {
  // Delete any existing save file to ensure consistent test state
  auto userId_opt = afterhours::EntityHelper::get_singleton<UserId>();
  if (userId_opt.get().has<UserId>()) {
    std::string userId = userId_opt.get().get<UserId>().userId;
    std::string save_file =
        server::FileStorage::get_game_state_save_path(userId);
    if (server::FileStorage::file_exists(save_file)) {
      std::filesystem::remove(save_file);
    }
  }
  
  app.launch_game();
  app.wait_for_frames(50); // Give UI systems plenty of time to create elements
  
  // Without a save file, we should see "Play" button
  // With a save file, we'd see "New Team" and "Continue" instead
  // Since we deleted the save file, we should see "Play"
  app.wait_for_ui_exists("Play", 15.0f);
  
  // These should always be present
  app.wait_for_ui_exists("Settings", 15.0f);
  app.wait_for_ui_exists("Dishes", 15.0f);
  app.wait_for_ui_exists("Quit", 15.0f);
}
