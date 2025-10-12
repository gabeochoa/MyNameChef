#pragma once

#include "../components/battle_load_request.h"
#include "../components/battle_result.h"
#include "../game_state_manager.h"
#include <afterhours/ah.h>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

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
      create_default_results(result);
    }

    // Check if BattleResult singleton already exists
    const auto componentId = afterhours::components::get_type_id<BattleResult>();
    bool singletonExists = afterhours::EntityHelper::get().singletonMap.contains(componentId);

    if (singletonExists) {
      // Update existing singleton
      auto existingResult = afterhours::EntityHelper::get_singleton<BattleResult>();
      if (existingResult.get().has<BattleResult>()) {
        auto &existingBattleResult = existingResult.get().get<BattleResult>();
        existingBattleResult.outcome = result.outcome;
        existingBattleResult.totalPlayerScore = result.totalPlayerScore;
        existingBattleResult.totalOpponentScore = result.totalOpponentScore;
        existingBattleResult.judgeScores = std::move(result.judgeScores);
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
    if (j.contains("totalScores")) {
      auto ts = j["totalScores"];
      if (ts.contains("player"))
        out.totalPlayerScore = ts["player"].get<int>();
      if (ts.contains("opponent"))
        out.totalOpponentScore = ts["opponent"].get<int>();
    }
    if (j.contains("judgeScores") && j["judgeScores"].is_array()) {
      for (const auto &js : j["judgeScores"]) {
        BattleResult::JudgeScore s;
        if (js.contains("judgeName"))
          s.judgeName = js["judgeName"].get<std::string>();
        if (js.contains("playerScore"))
          s.playerScore = js["playerScore"].get<int>();
        if (js.contains("opponentScore"))
          s.opponentScore = js["opponentScore"].get<int>();
        out.judgeScores.push_back(s);
      }
    }
    return true;
  }

  void create_default_results(BattleResult &out) {
    for (int i = 0; i < 3; ++i) {
      BattleResult::JudgeScore s;
      s.judgeName = "Judge " + std::to_string(i + 1);
      s.playerScore = 5 + i;
      s.opponentScore = 4 + i;
      out.totalPlayerScore += s.playerScore;
      out.totalOpponentScore += s.opponentScore;
      out.judgeScores.push_back(s);
    }
    if (out.totalPlayerScore > out.totalOpponentScore)
      out.outcome = BattleResult::Outcome::PlayerWin;
    else if (out.totalOpponentScore > out.totalPlayerScore)
      out.outcome = BattleResult::Outcome::OpponentWin;
    else
      out.outcome = BattleResult::Outcome::Tie;
  }
};
