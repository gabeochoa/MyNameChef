#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_result.h"
#include "../components/battle_team_tags.h"
#include "../components/is_dish.h"
#include "../dish_types.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>
#include <random>

struct LoadBattleResults : afterhours::System<> {
  bool loaded = false;
  GameStateManager::Screen last_screen = GameStateManager::Screen::Main;

  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();

    // Reset loaded flag when leaving results screen
    if (last_screen == GameStateManager::Screen::Results &&
        gsm.active_screen != GameStateManager::Screen::Results) {
      loaded = false;
    }

    last_screen = gsm.active_screen;
    return gsm.active_screen == GameStateManager::Screen::Results && !loaded;
  }

  void once(float) override {
    auto reqEnt = afterhours::EntityHelper::get_singleton<BattleLoadRequest>();
    if (!reqEnt.get().has<BattleLoadRequest>()) {
      log_error("No BattleLoadRequest found for loading results");
      return;
    }

    auto &req = reqEnt.get().get<BattleLoadRequest>();

    std::string resultPath = req.playerJsonPath;
    const auto pos = resultPath.find("pending");
    if (pos != std::string::npos)
      resultPath.replace(pos, 7, "results");

    BattleResult result;
    if (!load_results_from_json(resultPath, result)) {
      calculate_results_from_teams(result);
    }

    // Check if BattleResult singleton already exists
    const auto componentId =
        afterhours::components::get_type_id<BattleResult>();
    bool singletonExists =
        afterhours::EntityHelper::get().singletonMap.contains(componentId);

    if (singletonExists) {
      // Update existing singleton
      auto existingResult =
          afterhours::EntityHelper::get_singleton<BattleResult>();
      if (existingResult.get().has<BattleResult>()) {
        auto &existingBattleResult = existingResult.get().get<BattleResult>();
        existingBattleResult.outcome = result.outcome;
        existingBattleResult.playerWins = result.playerWins;
        existingBattleResult.opponentWins = result.opponentWins;
        existingBattleResult.ties = result.ties;
        existingBattleResult.outcomes = std::move(result.outcomes);
      } else {
        // Add component to existing entity
        existingResult.get().addComponent<BattleResult>(std::move(result));
      }
    } else {
      // Create new singleton
      auto &ent = afterhours::EntityHelper::createEntity();
      ent.addComponent<BattleResult>(std::move(result));
      afterhours::EntityHelper::registerSingleton<BattleResult>(ent);
    }
    loaded = true;
  }

private:
  bool load_results_from_json(const std::string &jsonPath, BattleResult &out) {
    if (!std::filesystem::exists(jsonPath))
      return false;
    std::ifstream f(jsonPath);
    if (!f.is_open())
      return false;
    nlohmann::json j;
    try {
      f >> j;
    } catch (...) {
      return false;
    }
    if (j.contains("outcome")) {
      const std::string s = j["outcome"].get<std::string>();
      if (s == "player_win")
        out.outcome = BattleResult::Outcome::PlayerWin;
      else if (s == "opponent_win")
        out.outcome = BattleResult::Outcome::OpponentWin;
      else
        out.outcome = BattleResult::Outcome::Tie;
    }
    // Ignore legacy totals/judge breakdown in new model
    return true;
  }

  void calculate_results_from_teams(BattleResult &out) {
    log_info("Calculating battle results from actual teams");

    // Get player team dishes
    std::vector<std::reference_wrapper<afterhours::Entity>> playerEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsPlayerTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      playerEntities.push_back(ref);
    }

    // Get opponent team dishes
    std::vector<std::reference_wrapper<afterhours::Entity>> opponentEntities;
    for (auto &ref : afterhours::EntityQuery()
                         .template whereHasComponent<IsOpponentTeamItem>()
                         .template whereHasComponent<IsDish>()
                         .gen()) {
      opponentEntities.push_back(ref);
    }

    // Calculate team scores based on flavor stats
    int playerTeamScore = calculate_team_score(playerEntities);
    int opponentTeamScore = calculate_team_score(opponentEntities);

    log_info("Player team score: {}, Opponent team score: {}", playerTeamScore,
             opponentTeamScore);

    // Determine simple outcome (placeholder until H2H loop)
    if (playerTeamScore > opponentTeamScore) {
      out.outcome = BattleResult::Outcome::PlayerWin;
      out.playerWins = 1;
      out.opponentWins = 0;
      out.ties = 0;
    } else if (opponentTeamScore > playerTeamScore) {
      out.outcome = BattleResult::Outcome::OpponentWin;
      out.playerWins = 0;
      out.opponentWins = 1;
      out.ties = 0;
    } else {
      out.outcome = BattleResult::Outcome::Tie;
      out.playerWins = 0;
      out.opponentWins = 0;
      out.ties = 1;
    }

    // Create course outcomes (simplified - one course per team)
    // In a real battle, this would be course-by-course comparison
    BattleResult::CourseOutcome courseOutcome;
    courseOutcome.slotIndex = 0; // Single course
    courseOutcome.ticks = 0;     // No timing in simplified version

    if (playerTeamScore > opponentTeamScore) {
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Player;
    } else if (opponentTeamScore > playerTeamScore) {
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Opponent;
    } else {
      courseOutcome.winner = BattleResult::CourseOutcome::Winner::Tie;
    }

    out.outcomes.push_back(courseOutcome);
  }

  int calculate_team_score(
      const std::vector<std::reference_wrapper<afterhours::Entity>>
          &teamEntities) {
    int totalScore = 0;

    for (const auto &entity_ref : teamEntities) {
      auto &entity = entity_ref.get();
      auto &dish = entity.get<IsDish>();

      // Get dish info and calculate score based on flavor stats
      DishInfo dishInfo = get_dish_info(dish.type);
      FlavorStats &flavor = dishInfo.flavor;

      // Calculate dish score as sum of all flavor stats
      int dishScore = flavor.satiety + flavor.sweetness + flavor.spice +
                      flavor.acidity + flavor.umami + flavor.richness +
                      flavor.freshness;

      totalScore += dishScore;

      log_info("Dish {} contributes {} points (satiety:{}, sweetness:{}, "
               "spice:{}, acidity:{}, umami:{}, richness:{}, freshness:{})",
               dishInfo.name, dishScore, flavor.satiety, flavor.sweetness,
               flavor.spice, flavor.acidity, flavor.umami, flavor.richness,
               flavor.freshness);
    }

    return totalScore;
  }
};
