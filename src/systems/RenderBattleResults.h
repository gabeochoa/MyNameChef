// Duplicate removed

#pragma once

#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../game_state_manager.h"
#include "../rl.h"
#include <afterhours/ah.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>

struct RenderBattleResults : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Results;
  }

  void once(float) const override {
    auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
    auto &entity = resultEntity.get();

    if (!entity.has<BattleResult>()) {
      return;
    }

    auto &result = entity.get<BattleResult>();

    // Debug: Print teams to console (commented to reduce spam)
    // print_teams_debug();

    float screenWidth = raylib::GetScreenWidth();

    std::string title = "BATTLE RESULTS";
    float titleWidth = raylib::MeasureText(title.c_str(), 48);
    float titleX = (screenWidth - titleWidth) / 2.0f;
    float titleY = 100.0f;

    raylib::DrawText(title.c_str(), (int)titleX, (int)titleY, 48,
                     raylib::YELLOW);

    std::string outcomeText;
    raylib::Color outcomeColor;

    switch (result.outcome) {
    case BattleResult::Outcome::PlayerWin:
      outcomeText = "PLAYER WINS!";
      outcomeColor = raylib::GREEN;
      break;
    case BattleResult::Outcome::OpponentWin:
      outcomeText = "OPPONENT WINS!";
      outcomeColor = raylib::RED;
      break;
    case BattleResult::Outcome::Tie:
      outcomeText = "IT'S A TIE!";
      outcomeColor = raylib::YELLOW;
      break;
    }

    float outcomeWidth = raylib::MeasureText(outcomeText.c_str(), 36);
    float outcomeX = (screenWidth - outcomeWidth) / 2.0f;
    float outcomeY = titleY + 80.0f;

    raylib::DrawText(outcomeText.c_str(), (int)outcomeX, (int)outcomeY, 36,
                     outcomeColor);

    // Display teams used in battle
    float teamsY = outcomeY + 60.0f;
    render_teams(screenWidth, teamsY);
  }

private:
  void print_teams_debug() const {
    log_info("=== BATTLE TEAMS DEBUG ===");

    // Get player team
    std::vector<std::string> playerDishes;
    std::vector<std::reference_wrapper<afterhours::Entity>> playerEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsPlayerTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      playerEntities.push_back(ref);
    }

    for (const auto &entity_ref : playerEntities) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();
      playerDishes.push_back(std::string(magic_enum::enum_name(dish.type)));
    }

    // Get opponent team
    std::vector<std::string> opponentDishes;
    std::vector<std::reference_wrapper<afterhours::Entity>> opponentEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsOpponentTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      opponentEntities.push_back(ref);
    }

    for (const auto &entity_ref : opponentEntities) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();
      opponentDishes.push_back(std::string(magic_enum::enum_name(dish.type)));
    }

    log_info("Player team ({}) dishes:", playerDishes.size());
    for (size_t i = 0; i < playerDishes.size(); ++i) {
      log_info("  Slot {}: {}", i, playerDishes[i]);
    }

    log_info("Opponent team ({}) dishes:", opponentDishes.size());
    for (size_t i = 0; i < opponentDishes.size(); ++i) {
      log_info("  Slot {}: {}", i, opponentDishes[i]);
    }

    log_info("=== END BATTLE TEAMS DEBUG ===");
  }

  void render_teams(float /* screenWidth */, float startY) const {
    // Player team
    std::string playerLabel = "PLAYER TEAM:";
    raylib::DrawText(playerLabel.c_str(), 50, (int)startY, 20, raylib::GREEN);

    float playerX = 50.0f;
    float playerY = startY + 30.0f;

    std::vector<std::reference_wrapper<afterhours::Entity>> playerEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsPlayerTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      playerEntities.push_back(ref);
    }

    for (const auto &entity_ref : playerEntities) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();
      std::string dishName = dish.name();

      raylib::DrawText(dishName.c_str(), (int)playerX, (int)playerY, 16,
                       raylib::WHITE);
      playerX += raylib::MeasureText(dishName.c_str(), 16) + 20.0f;
    }

    // Opponent team
    std::string opponentLabel = "OPPONENT TEAM:";
    raylib::DrawText(opponentLabel.c_str(), 50, (int)(startY + 60), 20,
                     raylib::RED);

    float opponentX = 50.0f;
    float opponentY = startY + 90.0f;

    std::vector<std::reference_wrapper<afterhours::Entity>> opponentEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsOpponentTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      opponentEntities.push_back(ref);
    }

    for (const auto &entity_ref : opponentEntities) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();
      std::string dishName = dish.name();

      raylib::DrawText(dishName.c_str(), (int)opponentX, (int)opponentY, 16,
                       raylib::WHITE);
      opponentX += raylib::MeasureText(dishName.c_str(), 16) + 20.0f;
    }
  }
};
