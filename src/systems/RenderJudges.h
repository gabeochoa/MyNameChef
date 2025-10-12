#pragma once

#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>

struct RenderJudges : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void once(float) override {
    log_info("RenderJudges system running on battle screen");

    // Draw 3 judge rectangles centered
    float screenWidth = raylib::GetScreenWidth();
    float screenHeight = raylib::GetScreenHeight();

    // Judge dimensions
    float judgeWidth = 60.0f;
    float judgeHeight = 100.0f;
    float judgeSpacing = 20.0f;

    // Calculate total width and starting position
    float totalWidth = (judgeWidth * 3) + (judgeSpacing * 2);
    float startX = (screenWidth - totalWidth) / 2.0f;
    float judgeY =
        screenHeight / 2.0f - judgeHeight / 2.0f; // Center vertically

    // Draw the 3 judges
    for (int i = 0; i < 3; ++i) {
      float judgeX = startX + i * (judgeWidth + judgeSpacing);

      // Draw judge rectangle
      raylib::DrawRectangle((int)judgeX, (int)judgeY, (int)judgeWidth,
                            (int)judgeHeight,
                            raylib::Color{100, 100, 100, 255} // Gray color
      );

      // Draw border
      raylib::DrawRectangleLines((int)judgeX, (int)judgeY, (int)judgeWidth,
                                 (int)judgeHeight, raylib::WHITE);

      // Draw judge number
      std::string judgeNumber = std::to_string(i + 1);
      float textWidth = raylib::MeasureText(judgeNumber.c_str(), 20);
      float textX = judgeX + (judgeWidth - textWidth) / 2.0f;
      float textY = judgeY + (judgeHeight - 20) / 2.0f;

      raylib::DrawText(judgeNumber.c_str(), (int)textX, (int)textY, 20,
                       raylib::WHITE);
    }

    // Draw "JUDGES" title above the judges
    std::string title = "JUDGES";
    float titleWidth = raylib::MeasureText(title.c_str(), 24);
    float titleX = (screenWidth - titleWidth) / 2.0f;
    float titleY = judgeY - 40.0f;

    raylib::DrawText(title.c_str(), (int)titleX, (int)titleY, 24,
                     raylib::YELLOW);
  }
};
