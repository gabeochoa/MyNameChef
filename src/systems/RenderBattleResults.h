// Duplicate removed

#pragma once

#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../font_info.h"
#include "../game_state_manager.h"
#include "../render_backend.h"
#include "../rl.h"
#include "../ui/text_formatting.h"
#include <afterhours/ah.h>
#include <functional>
#include <magic_enum/magic_enum.hpp>

struct RenderBattleResults : afterhours::System<> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.active_screen == GameStateManager::Screen::Results;
  }

  void once(float) const override {
    // Check if singleton exists before trying to get it
    const auto componentId =
        afterhours::components::get_type_id<BattleResult>();
    if (!afterhours::EntityHelper::get().singletonMap.contains(componentId)) {
      return;
    }

    auto resultEntity = afterhours::EntityHelper::get_singleton<BattleResult>();
    auto &entity = resultEntity.get();

    // Check if entity is valid and has the component before accessing
    if (entity.cleanup || !entity.has<BattleResult>()) {
      return;
    }

    auto &result = entity.get<BattleResult>();

    // (quiet)

    float screenWidth = raylib::GetScreenWidth();

    std::string title = "BATTLE RESULTS";
    float titleWidth = render_backend::MeasureTextWithActiveFont(title.c_str(), font_sizes::Title);
    float titleX = (screenWidth - titleWidth) / 2.0f;
    float titleY = 100.0f;

    raylib::Color title_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Accent,
        text_formatting::FormattingContext::Results);
    render_backend::DrawTextWithActiveFont(title.c_str(), (int)titleX, (int)titleY, font_sizes::Title,
                                          title_color);

    std::string outcomeText;
    raylib::Color outcomeColor;

    switch (result.outcome) {
    case BattleResult::Outcome::PlayerWin:
      outcomeText = "PLAYER WINS!";
      outcomeColor = text_formatting::TextFormatting::get_color(
          text_formatting::SemanticColor::Success,
          text_formatting::FormattingContext::Results);
      break;
    case BattleResult::Outcome::OpponentWin:
      outcomeText = "OPPONENT WINS!";
      outcomeColor = text_formatting::TextFormatting::get_color(
          text_formatting::SemanticColor::Error,
          text_formatting::FormattingContext::Results);
      break;
    case BattleResult::Outcome::Tie:
      outcomeText = "IT'S A TIE!";
      outcomeColor = text_formatting::TextFormatting::get_color(
          text_formatting::SemanticColor::Warning,
          text_formatting::FormattingContext::Results);
      break;
    }

    float outcomeWidth = render_backend::MeasureTextWithActiveFont(outcomeText.c_str(), font_sizes::Large);
    float outcomeX = (screenWidth - outcomeWidth) / 2.0f;
    float outcomeY = titleY + 80.0f;

    render_backend::DrawTextWithActiveFont(outcomeText.c_str(), (int)outcomeX, (int)outcomeY, font_sizes::Large,
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
    raylib::Color text_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Text,
        text_formatting::FormattingContext::Results);
    raylib::Color success_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Success,
        text_formatting::FormattingContext::Results);
    raylib::Color error_color = text_formatting::TextFormatting::get_color(
        text_formatting::SemanticColor::Error,
        text_formatting::FormattingContext::Results);

    // Player team
    std::string playerLabel = "PLAYER TEAM:";
    render_backend::DrawTextWithActiveFont(playerLabel.c_str(), 50, (int)startY, font_sizes::Medium, success_color);

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

      render_backend::DrawTextWithActiveFont(dishName.c_str(), (int)playerX, (int)playerY, font_sizes::Normal,
                                            text_color);
      playerX += render_backend::MeasureTextWithActiveFont(dishName.c_str(), font_sizes::Normal) + 20.0f;
    }

    // Opponent team
    std::string opponentLabel = "OPPONENT TEAM:";
    render_backend::DrawTextWithActiveFont(opponentLabel.c_str(), 50, (int)(startY + 60), font_sizes::Medium,
                                          error_color);

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

      render_backend::DrawTextWithActiveFont(dishName.c_str(), (int)opponentX, (int)opponentY, font_sizes::Normal,
                                            text_color);
      opponentX += render_backend::MeasureTextWithActiveFont(dishName.c_str(), font_sizes::Normal) + 20.0f;
    }
  }
};
