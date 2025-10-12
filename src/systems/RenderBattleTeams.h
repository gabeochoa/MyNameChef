#pragma once

#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>

struct RenderBattleTeams : afterhours::System<Transform, IsDish> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Battle;
  }

  void for_each_with(const afterhours::Entity &entity,
                     const Transform &transform, const IsDish &dish,
                     float) const override {

    // Only render entities that have battle team tags
    bool isPlayer = entity.has<IsPlayerTeamItem>();
    bool isOpponent = entity.has<IsOpponentTeamItem>();

    if (!isPlayer && !isOpponent) {
      return; // Not a battle team item, skip
    }

    // Get dish color
    raylib::Color dishColor = dish.color();

    // Draw the dish rectangle
    raylib::DrawRectangle((int)transform.position.x, (int)transform.position.y,
                          (int)transform.size.x, (int)transform.size.y,
                          dishColor);

    // Draw border
    raylib::Color borderColor = isPlayer ? raylib::GREEN : raylib::RED;
    raylib::DrawRectangleLines((int)transform.position.x,
                               (int)transform.position.y, (int)transform.size.x,
                               (int)transform.size.y, borderColor);

    // Draw dish name above the rectangle
    std::string dishName = dish.name();
    float textWidth = raylib::MeasureText(dishName.c_str(), 16);
    float textX = transform.position.x + (transform.size.x - textWidth) / 2.0f;
    float textY = transform.position.y - 25.0f;

    raylib::DrawText(dishName.c_str(), (int)textX, (int)textY, 16,
                     raylib::WHITE);

    // Draw team label
    std::string teamLabel = isPlayer ? "PLAYER" : "OPPONENT";
    float labelWidth = raylib::MeasureText(teamLabel.c_str(), 12);
    float labelX =
        transform.position.x + (transform.size.x - labelWidth) / 2.0f;
    float labelY = transform.position.y + transform.size.y + 5.0f;

    raylib::DrawText(teamLabel.c_str(), (int)labelX, (int)labelY, 12,
                     borderColor);
  }
};
